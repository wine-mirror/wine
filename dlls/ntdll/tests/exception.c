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
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winreg.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "excpt.h"
#include "wine/test.h"
#include "intrin.h"

static void *code_mem;

static NTSTATUS  (WINAPI *pNtGetContextThread)(HANDLE,CONTEXT*);
static NTSTATUS  (WINAPI *pNtSetContextThread)(HANDLE,CONTEXT*);
static NTSTATUS  (WINAPI *pNtQueueApcThread)(HANDLE handle, PNTAPCFUNC func,
        ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3);
static NTSTATUS  (WINAPI *pRtlRaiseException)(EXCEPTION_RECORD *rec);
static PVOID     (WINAPI *pRtlUnwind)(PVOID, PVOID, PEXCEPTION_RECORD, PVOID);
static VOID      (WINAPI *pRtlCaptureContext)(CONTEXT*);
static PVOID     (WINAPI *pRtlAddVectoredExceptionHandler)(ULONG first, PVECTORED_EXCEPTION_HANDLER func);
static ULONG     (WINAPI *pRtlRemoveVectoredExceptionHandler)(PVOID handler);
static PVOID     (WINAPI *pRtlAddVectoredContinueHandler)(ULONG first, PVECTORED_EXCEPTION_HANDLER func);
static ULONG     (WINAPI *pRtlRemoveVectoredContinueHandler)(PVOID handler);
static void      (WINAPI *pRtlSetUnhandledExceptionFilter)(PRTL_EXCEPTION_FILTER filter);
static ULONG64   (WINAPI *pRtlGetEnabledExtendedFeatures)(ULONG64);
static NTSTATUS  (WINAPI *pRtlGetExtendedContextLength)(ULONG context_flags, ULONG *length);
static NTSTATUS  (WINAPI *pRtlGetExtendedContextLength2)(ULONG context_flags, ULONG *length, ULONG64 compaction_mask);
static NTSTATUS  (WINAPI *pRtlInitializeExtendedContext)(void *context, ULONG context_flags, CONTEXT_EX **context_ex);
static NTSTATUS  (WINAPI *pRtlInitializeExtendedContext2)(void *context, ULONG context_flags, CONTEXT_EX **context_ex,
        ULONG64 compaction_mask);
static NTSTATUS  (WINAPI *pRtlCopyContext)(CONTEXT *dst, DWORD context_flags, CONTEXT *src);
static NTSTATUS  (WINAPI *pRtlCopyExtendedContext)(CONTEXT_EX *dst, ULONG context_flags, CONTEXT_EX *src);
static void *    (WINAPI *pRtlLocateExtendedFeature)(CONTEXT_EX *context_ex, ULONG feature_id, ULONG *length);
static void *    (WINAPI *pRtlLocateLegacyContext)(CONTEXT_EX *context_ex, ULONG *length);
static void      (WINAPI *pRtlSetExtendedFeaturesMask)(CONTEXT_EX *context_ex, ULONG64 feature_mask);
static ULONG64   (WINAPI *pRtlGetExtendedFeaturesMask)(CONTEXT_EX *context_ex);
static NTSTATUS  (WINAPI *pNtRaiseException)(EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance);
static NTSTATUS  (WINAPI *pNtReadVirtualMemory)(HANDLE, const void*, void*, SIZE_T, SIZE_T*);
static NTSTATUS  (WINAPI *pNtTerminateProcess)(HANDLE handle, LONG exit_code);
static NTSTATUS  (WINAPI *pNtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
static NTSTATUS  (WINAPI *pNtQueryInformationThread)(HANDLE, THREADINFOCLASS, PVOID, ULONG, PULONG);
static NTSTATUS  (WINAPI *pNtSetInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG);
static BOOL      (WINAPI *pIsWow64Process)(HANDLE, PBOOL);
static NTSTATUS  (WINAPI *pNtClose)(HANDLE);
static NTSTATUS  (WINAPI *pNtSuspendProcess)(HANDLE process);
static NTSTATUS  (WINAPI *pNtResumeProcess)(HANDLE process);
static BOOL      (WINAPI *pInitializeContext)(void *buffer, DWORD context_flags, CONTEXT **context,
        DWORD *length);
static BOOL      (WINAPI *pInitializeContext2)(void *buffer, DWORD context_flags, CONTEXT **context,
        DWORD *length, ULONG64 compaction_mask);
static void *    (WINAPI *pLocateXStateFeature)(CONTEXT *context, DWORD feature_id, DWORD *length);
static BOOL      (WINAPI *pSetXStateFeaturesMask)(CONTEXT *context, DWORD64 feature_mask);
static BOOL      (WINAPI *pGetXStateFeaturesMask)(CONTEXT *context, DWORD64 *feature_mask);

#define RTL_UNLOAD_EVENT_TRACE_NUMBER 64

typedef struct _RTL_UNLOAD_EVENT_TRACE
{
    void *BaseAddress;
    SIZE_T SizeOfImage;
    ULONG Sequence;
    ULONG TimeDateStamp;
    ULONG CheckSum;
    WCHAR ImageName[32];
} RTL_UNLOAD_EVENT_TRACE, *PRTL_UNLOAD_EVENT_TRACE;

static RTL_UNLOAD_EVENT_TRACE *(WINAPI *pRtlGetUnloadEventTrace)(void);
static void (WINAPI *pRtlGetUnloadEventTraceEx)(ULONG **element_size, ULONG **element_count, void **event_trace);

#ifndef EH_NESTED_CALL
#define EH_NESTED_CALL 0x10
#endif

#if defined(__x86_64__)
typedef struct
{
    ULONG Count;
    struct
    {
        ULONG BeginAddress;
        ULONG EndAddress;
        ULONG HandlerAddress;
        ULONG JumpTarget;
    } ScopeRecord[1];
} SCOPE_TABLE;

typedef struct _SETJMP_FLOAT128
{
    unsigned __int64 DECLSPEC_ALIGN(16) Part[2];
} SETJMP_FLOAT128;

typedef struct _JUMP_BUFFER
{
    unsigned __int64 Frame;
    unsigned __int64 Rbx;
    unsigned __int64 Rsp;
    unsigned __int64 Rbp;
    unsigned __int64 Rsi;
    unsigned __int64 Rdi;
    unsigned __int64 R12;
    unsigned __int64 R13;
    unsigned __int64 R14;
    unsigned __int64 R15;
    unsigned __int64 Rip;
    unsigned long MxCsr;
    unsigned short FpCsr;
    unsigned short Spare;
    SETJMP_FLOAT128  Xmm6;
    SETJMP_FLOAT128  Xmm7;
    SETJMP_FLOAT128  Xmm8;
    SETJMP_FLOAT128  Xmm9;
    SETJMP_FLOAT128  Xmm10;
    SETJMP_FLOAT128  Xmm11;
    SETJMP_FLOAT128  Xmm12;
    SETJMP_FLOAT128  Xmm13;
    SETJMP_FLOAT128  Xmm14;
    SETJMP_FLOAT128  Xmm15;
} _JUMP_BUFFER;

typedef union _UNWIND_CODE
{
    struct
    {
        BYTE CodeOffset;
        BYTE UnwindOp : 4;
        BYTE OpInfo   : 4;
    } s;
    USHORT FrameOffset;
} UNWIND_CODE;

typedef struct _UNWIND_INFO
{
    BYTE Version       : 3;
    BYTE Flags         : 5;
    BYTE SizeOfProlog;
    BYTE CountOfCodes;
    BYTE FrameRegister : 4;
    BYTE FrameOffset   : 4;
    UNWIND_CODE UnwindCode[1]; /* actually CountOfCodes (aligned) */
/*
 *  union
 *  {
 *      OPTIONAL ULONG ExceptionHandler;
 *      OPTIONAL ULONG FunctionEntry;
 *  };
 *  OPTIONAL ULONG ExceptionData[];
 */
} UNWIND_INFO;

static BOOLEAN   (CDECL *pRtlAddFunctionTable)(RUNTIME_FUNCTION*, DWORD, DWORD64);
static BOOLEAN   (CDECL *pRtlDeleteFunctionTable)(RUNTIME_FUNCTION*);
static BOOLEAN   (CDECL *pRtlInstallFunctionTableCallback)(DWORD64, DWORD64, DWORD, PGET_RUNTIME_FUNCTION_CALLBACK, PVOID, PCWSTR);
static PRUNTIME_FUNCTION (WINAPI *pRtlLookupFunctionEntry)(ULONG64, ULONG64*, UNWIND_HISTORY_TABLE*);
static DWORD     (WINAPI *pRtlAddGrowableFunctionTable)(void**, RUNTIME_FUNCTION*, DWORD, DWORD, ULONG_PTR, ULONG_PTR);
static void      (WINAPI *pRtlGrowFunctionTable)(void*, DWORD);
static void      (WINAPI *pRtlDeleteGrowableFunctionTable)(void*);
static EXCEPTION_DISPOSITION (WINAPI *p__C_specific_handler)(EXCEPTION_RECORD*, ULONG64, CONTEXT*, DISPATCHER_CONTEXT*);
static VOID      (WINAPI *pRtlCaptureContext)(CONTEXT*);
static VOID      (CDECL *pRtlRestoreContext)(CONTEXT*, EXCEPTION_RECORD*);
static NTSTATUS  (WINAPI *pRtlWow64GetThreadContext)(HANDLE, WOW64_CONTEXT *);
static NTSTATUS  (WINAPI *pRtlWow64SetThreadContext)(HANDLE, const WOW64_CONTEXT *);
static NTSTATUS  (WINAPI *pRtlWow64GetCpuAreaInfo)(WOW64_CPURESERVED*,ULONG,WOW64_CPU_AREA_INFO*);
static VOID      (WINAPI *pRtlUnwindEx)(VOID*, VOID*, EXCEPTION_RECORD*, VOID*, CONTEXT*, UNWIND_HISTORY_TABLE*);
static int       (CDECL *p_setjmp)(_JUMP_BUFFER*);
#endif

static int      my_argc;
static char**   my_argv;
static BOOL     is_wow64;
static BOOL have_vectored_api;
static int test_stage;

#if defined(__i386__) || defined(__x86_64__)
static void test_debugger_xstate(HANDLE thread, CONTEXT *ctx, int stage)
{
    char context_buffer[sizeof(CONTEXT) + sizeof(CONTEXT_EX) + sizeof(XSTATE) + 63];
    CONTEXT_EX *c_ex;
    NTSTATUS status;
    YMMCONTEXT *ymm;
    CONTEXT *xctx;
    DWORD length;
    XSTATE *xs;
    M128A *xmm;
    BOOL bret;

    if (!pRtlGetEnabledExtendedFeatures || !pRtlGetEnabledExtendedFeatures(1 << XSTATE_AVX))
        return;

    if (stage == 14)
        return;

    length = sizeof(context_buffer);
    bret = pInitializeContext(context_buffer, ctx->ContextFlags | CONTEXT_XSTATE, &xctx, &length);
    ok(bret, "Got unexpected bret %#x, GetLastError() %u.\n", bret, GetLastError());

    ymm = pLocateXStateFeature(xctx, XSTATE_AVX, &length);
    ok(!!ymm, "Got zero ymm.\n");
    memset(ymm, 0xcc, sizeof(*ymm));

    xmm = pLocateXStateFeature(xctx, XSTATE_LEGACY_SSE, &length);
    ok(length == sizeof(*xmm) * (sizeof(void *) == 8 ? 16 : 8), "Got unexpected length %#x.\n", length);
    ok(!!xmm, "Got zero xmm.\n");
    memset(xmm, 0xcc, length);

    status = pNtGetContextThread(thread, xctx);
    ok(!status, "NtSetContextThread failed with 0x%x\n", status);

    c_ex = (CONTEXT_EX *)(xctx + 1);
    xs = (XSTATE *)((char *)c_ex + c_ex->XState.Offset);
    ok((xs->Mask & 7) == 4 || broken(!xs->Mask) /* Win7 */,
            "Got unexpected xs->Mask %s.\n", wine_dbgstr_longlong(xs->Mask));

    ok(xmm[0].Low == 0x200000001, "Got unexpected data %s.\n", wine_dbgstr_longlong(xmm[0].Low));
    ok(xmm[0].High == 0x400000003, "Got unexpected data %s.\n", wine_dbgstr_longlong(xmm[0].High));

    ok(ymm->Ymm0.Low == 0x600000005 || broken(!xs->Mask && ymm->Ymm0.Low == 0xcccccccccccccccc) /* Win7 */,
            "Got unexpected data %s.\n", wine_dbgstr_longlong(ymm->Ymm0.Low));
    ok(ymm->Ymm0.High == 0x800000007 || broken(!xs->Mask && ymm->Ymm0.High == 0xcccccccccccccccc) /* Win7 */,
            "Got unexpected data %s.\n", wine_dbgstr_longlong(ymm->Ymm0.High));

    xmm = pLocateXStateFeature(ctx, XSTATE_LEGACY_SSE, &length);
    ok(!!xmm, "Got zero xmm.\n");

    xmm[0].Low = 0x2828282828282828;
    xmm[0].High = xmm[0].Low;
    ymm->Ymm0.Low = 0x4848484848484848;
    ymm->Ymm0.High = ymm->Ymm0.Low;

    status = pNtSetContextThread(thread, xctx);
    ok(!status, "NtSetContextThread failed with 0x%x\n", status);
}
#endif

#ifdef __i386__

#ifndef __WINE_WINTRNL_H
#define ProcessExecuteFlags 0x22
#define MEM_EXECUTE_OPTION_DISABLE   0x01
#define MEM_EXECUTE_OPTION_ENABLE    0x02
#define MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION 0x04
#define MEM_EXECUTE_OPTION_PERMANENT 0x08
#endif

/* Test various instruction combinations that cause a protection fault on the i386,
 * and check what the resulting exception looks like.
 */

static const struct exception
{
    BYTE     code[18];      /* asm code */
    BYTE     offset;        /* offset of faulting instruction */
    BYTE     length;        /* length of faulting instruction */
    BOOL     wow64_broken;  /* broken on Wow64, should be skipped */
    NTSTATUS status;        /* expected status code */
    DWORD    nb_params;     /* expected number of parameters */
    DWORD    params[4];     /* expected parameters */
    NTSTATUS alt_status;    /* alternative status code */
    DWORD    alt_nb_params; /* alternative number of parameters */
    DWORD    alt_params[4]; /* alternative parameters */
} exceptions[] =
{
/* 0 */
    /* test some privileged instructions */
    { { 0xfb, 0xc3 },  /* 0: sti; ret */
      0, 1, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x6c, 0xc3 },  /* 1: insb (%dx); ret */
      0, 1, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x6d, 0xc3 },  /* 2: insl (%dx); ret */
      0, 1, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x6e, 0xc3 },  /* 3: outsb (%dx); ret */
      0, 1, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x6f, 0xc3 },  /* 4: outsl (%dx); ret */
      0, 1, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
/* 5 */
    { { 0xe4, 0x11, 0xc3 },  /* 5: inb $0x11,%al; ret */
      0, 2, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xe5, 0x11, 0xc3 },  /* 6: inl $0x11,%eax; ret */
      0, 2, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xe6, 0x11, 0xc3 },  /* 7: outb %al,$0x11; ret */
      0, 2, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xe7, 0x11, 0xc3 },  /* 8: outl %eax,$0x11; ret */
      0, 2, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xed, 0xc3 },  /* 9: inl (%dx),%eax; ret */
      0, 1, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
/* 10 */
    { { 0xee, 0xc3 },  /* 10: outb %al,(%dx); ret */
      0, 1, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xef, 0xc3 },  /* 11: outl %eax,(%dx); ret */
      0, 1, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xf4, 0xc3 },  /* 12: hlt; ret */
      0, 1, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xfa, 0xc3 },  /* 13: cli; ret */
      0, 1, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },

    /* test long jump to invalid selector */
    { { 0xea, 0, 0, 0, 0, 0, 0, 0xc3 },  /* 14: ljmp $0,$0; ret */
      0, 7, FALSE, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },

/* 15 */
    /* test iret to invalid selector */
    { { 0x6a, 0x00, 0x6a, 0x00, 0x6a, 0x00, 0xcf, 0x83, 0xc4, 0x0c, 0xc3 },
      /* 15: pushl $0; pushl $0; pushl $0; iret; addl $12,%esp; ret */
      6, 1, FALSE, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },

    /* test loading an invalid selector */
    { { 0xb8, 0xef, 0xbe, 0x00, 0x00, 0x8e, 0xe8, 0xc3 },  /* 16: mov $beef,%ax; mov %ax,%gs; ret */
      5, 2, FALSE, STATUS_ACCESS_VIOLATION, 2, { 0, 0xbee8 } }, /* 0xbee8 or 0xffffffff */

    /* test accessing a zero selector (%es broken on Wow64) */
    { { 0x06, 0x31, 0xc0, 0x8e, 0xc0, 0x26, 0xa1, 0, 0, 0, 0, 0x07, 0xc3 },
       /* push %es; xor %eax,%eax; mov %ax,%es; mov %es:(0),%ax; pop %es; ret */
      5, 6, TRUE, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },
    { { 0x0f, 0xa8, 0x31, 0xc0, 0x8e, 0xe8, 0x65, 0xa1, 0, 0, 0, 0, 0x0f, 0xa9, 0xc3 },
      /* push %gs; xor %eax,%eax; mov %ax,%gs; mov %gs:(0),%ax; pop %gs; ret */
      6, 6, FALSE, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },

    /* test moving %cs -> %ss */
    { { 0x0e, 0x17, 0x58, 0xc3 },  /* pushl %cs; popl %ss; popl %eax; ret */
      1, 1, FALSE, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },

/* 20 */
    /* test overlong instruction (limit is 15 bytes, 5 on Win7) */
    { { 0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0xfa,0xc3 },
      0, 16, TRUE, STATUS_ILLEGAL_INSTRUCTION, 0, { 0 },
      STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },
    { { 0x64,0x64,0x64,0x64,0xfa,0xc3 },
      0, 5, TRUE, STATUS_PRIVILEGED_INSTRUCTION, 0 },

    /* test invalid interrupt */
    { { 0xcd, 0xff, 0xc3 },   /* int $0xff; ret */
      0, 2, FALSE, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },

    /* test moves to/from Crx */
    { { 0x0f, 0x20, 0xc0, 0xc3 },  /* movl %cr0,%eax; ret */
      0, 3, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x20, 0xe0, 0xc3 },  /* movl %cr4,%eax; ret */
      0, 3, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
/* 25 */
    { { 0x0f, 0x22, 0xc0, 0xc3 },  /* movl %eax,%cr0; ret */
      0, 3, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x22, 0xe0, 0xc3 },  /* movl %eax,%cr4; ret */
      0, 3, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },

    /* test moves to/from Drx */
    { { 0x0f, 0x21, 0xc0, 0xc3 },  /* movl %dr0,%eax; ret */
      0, 3, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x21, 0xc8, 0xc3 },  /* movl %dr1,%eax; ret */
      0, 3, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x21, 0xf8, 0xc3 },  /* movl %dr7,%eax; ret */
      0, 3, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
/* 30 */
    { { 0x0f, 0x23, 0xc0, 0xc3 },  /* movl %eax,%dr0; ret */
      0, 3, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x23, 0xc8, 0xc3 },  /* movl %eax,%dr1; ret */
      0, 3, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x23, 0xf8, 0xc3 },  /* movl %eax,%dr7; ret */
      0, 3, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },

    /* test memory reads */
    { { 0xa1, 0xfc, 0xff, 0xff, 0xff, 0xc3 },  /* movl 0xfffffffc,%eax; ret */
      0, 5, FALSE, STATUS_ACCESS_VIOLATION, 2, { 0, 0xfffffffc } },
    { { 0xa1, 0xfd, 0xff, 0xff, 0xff, 0xc3 },  /* movl 0xfffffffd,%eax; ret */
      0, 5, FALSE, STATUS_ACCESS_VIOLATION, 2, { 0, 0xfffffffd } },
/* 35 */
    { { 0xa1, 0xfe, 0xff, 0xff, 0xff, 0xc3 },  /* movl 0xfffffffe,%eax; ret */
      0, 5, FALSE, STATUS_ACCESS_VIOLATION, 2, { 0, 0xfffffffe } },
    { { 0xa1, 0xff, 0xff, 0xff, 0xff, 0xc3 },  /* movl 0xffffffff,%eax; ret */
      0, 5, FALSE, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },

    /* test memory writes */
    { { 0xa3, 0xfc, 0xff, 0xff, 0xff, 0xc3 },  /* movl %eax,0xfffffffc; ret */
      0, 5, FALSE, STATUS_ACCESS_VIOLATION, 2, { 1, 0xfffffffc } },
    { { 0xa3, 0xfd, 0xff, 0xff, 0xff, 0xc3 },  /* movl %eax,0xfffffffd; ret */
      0, 5, FALSE, STATUS_ACCESS_VIOLATION, 2, { 1, 0xfffffffd } },
    { { 0xa3, 0xfe, 0xff, 0xff, 0xff, 0xc3 },  /* movl %eax,0xfffffffe; ret */
      0, 5, FALSE, STATUS_ACCESS_VIOLATION, 2, { 1, 0xfffffffe } },
/* 40 */
    { { 0xa3, 0xff, 0xff, 0xff, 0xff, 0xc3 },  /* movl %eax,0xffffffff; ret */
      0, 5, FALSE, STATUS_ACCESS_VIOLATION, 2, { 1, 0xffffffff } },

    /* test exception with cleared %ds and %es (broken on Wow64) */
    { { 0x1e, 0x06, 0x31, 0xc0, 0x8e, 0xd8, 0x8e, 0xc0, 0xfa, 0x07, 0x1f, 0xc3 },
          /* push %ds; push %es; xorl %eax,%eax; mov %ax,%ds; mov %ax,%es; cli; pop %es; pop %ds; ret */
      8, 1, TRUE, STATUS_PRIVILEGED_INSTRUCTION, 0 },

    { { 0xf1, 0x90, 0xc3 },  /* icebp; nop; ret */
      1, 1, FALSE, STATUS_SINGLE_STEP, 0 },
    { { 0xb8, 0xb8, 0xb8, 0xb8, 0xb8,          /* mov $0xb8b8b8b8, %eax */
        0xb9, 0xb9, 0xb9, 0xb9, 0xb9,          /* mov $0xb9b9b9b9, %ecx */
        0xba, 0xba, 0xba, 0xba, 0xba,          /* mov $0xbabababa, %edx */
        0xcd, 0x2d, 0xc3 },                    /* int $0x2d; ret */
      17, 0, FALSE, STATUS_BREAKPOINT, 3, { 0xb8b8b8b8, 0xb9b9b9b9, 0xbabababa } },
};

static int got_exception;

static void run_exception_test(void *handler, const void* context,
                               const void *code, unsigned int code_size,
                               DWORD access)
{
    struct {
        EXCEPTION_REGISTRATION_RECORD frame;
        const void *context;
    } exc_frame;
    void (*func)(void) = code_mem;
    DWORD oldaccess, oldaccess2;

    exc_frame.frame.Handler = handler;
    exc_frame.frame.Prev = NtCurrentTeb()->Tib.ExceptionList;
    exc_frame.context = context;

    memcpy(code_mem, code, code_size);
    if(access)
        VirtualProtect(code_mem, code_size, access, &oldaccess);

    NtCurrentTeb()->Tib.ExceptionList = &exc_frame.frame;
    func();
    NtCurrentTeb()->Tib.ExceptionList = exc_frame.frame.Prev;

    if(access)
        VirtualProtect(code_mem, code_size, oldaccess, &oldaccess2);
}

static LONG CALLBACK rtlraiseexception_vectored_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT context = ExceptionInfo->ContextRecord;
    PEXCEPTION_RECORD rec = ExceptionInfo->ExceptionRecord;
    trace("vect. handler %08x addr:%p context.Eip:%x\n", rec->ExceptionCode,
          rec->ExceptionAddress, context->Eip);

    ok(rec->ExceptionAddress == (char *)code_mem + 0xb
            || broken(rec->ExceptionAddress == code_mem || !rec->ExceptionAddress) /* 2008 */,
            "ExceptionAddress at %p instead of %p\n", rec->ExceptionAddress, (char *)code_mem + 0xb);

    if (NtCurrentTeb()->Peb->BeingDebugged)
        ok((void *)context->Eax == pRtlRaiseException ||
           broken( is_wow64 && context->Eax == 0xf00f00f1 ), /* broken on vista */
           "debugger managed to modify Eax to %x should be %p\n",
           context->Eax, pRtlRaiseException);

    /* check that context.Eip is fixed up only for EXCEPTION_BREAKPOINT
     * even if raised by RtlRaiseException
     */
    if(rec->ExceptionCode == EXCEPTION_BREAKPOINT)
    {
        ok(context->Eip == (DWORD)code_mem + 0xa ||
           broken(context->Eip == (DWORD)code_mem + 0xb) /* win2k3 */ ||
           broken(context->Eip == (DWORD)code_mem + 0xd) /* w2008 */,
           "Eip at %x instead of %x or %x\n", context->Eip,
           (DWORD)code_mem + 0xa, (DWORD)code_mem + 0xb);
    }
    else
    {
        ok(context->Eip == (DWORD)code_mem + 0xb ||
           broken(context->Eip == (DWORD)code_mem + 0xd) /* w2008 */,
           "Eip at %x instead of %x\n", context->Eip, (DWORD)code_mem + 0xb);
    }

    /* test if context change is preserved from vectored handler to stack handlers */
    context->Eax = 0xf00f00f0;
    return EXCEPTION_CONTINUE_SEARCH;
}

static DWORD rtlraiseexception_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                      CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    trace( "exception: %08x flags:%x addr:%p context: Eip:%x\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress, context->Eip );

    ok(rec->ExceptionAddress == (char *)code_mem + 0xb
            || broken(rec->ExceptionAddress == code_mem || !rec->ExceptionAddress) /* 2008 */,
            "ExceptionAddress at %p instead of %p\n", rec->ExceptionAddress, (char *)code_mem + 0xb);

    ok( context->ContextFlags == CONTEXT_ALL || context->ContextFlags == (CONTEXT_ALL | CONTEXT_XSTATE) ||
        broken(context->ContextFlags == CONTEXT_FULL),  /* win2003 */
        "wrong context flags %x\n", context->ContextFlags );

    /* check that context.Eip is fixed up only for EXCEPTION_BREAKPOINT
     * even if raised by RtlRaiseException
     */
    if(rec->ExceptionCode == EXCEPTION_BREAKPOINT)
    {
        ok(context->Eip == (DWORD)code_mem + 0xa ||
           broken(context->Eip == (DWORD)code_mem + 0xb) /* win2k3 */ ||
           broken(context->Eip == (DWORD)code_mem + 0xd) /* w2008 */,
           "Eip at %x instead of %x or %x\n", context->Eip,
           (DWORD)code_mem + 0xa, (DWORD)code_mem + 0xb);
    }
    else
    {
        ok(context->Eip == (DWORD)code_mem + 0xb ||
           broken(context->Eip == (DWORD)code_mem + 0xd) /* w2008 */,
           "Eip at %x instead of %x\n", context->Eip, (DWORD)code_mem + 0xb);
    }

    if(have_vectored_api)
        ok(context->Eax == 0xf00f00f0, "Eax is %x, should have been set to 0xf00f00f0 in vectored handler\n",
           context->Eax);

    /* give the debugger a chance to examine the state a second time */
    /* without the exception handler changing Eip */
    if (test_stage == 2)
        return ExceptionContinueSearch;

    /* Eip in context is decreased by 1
     * Increase it again, else execution will continue in the middle of an instruction */
    if(rec->ExceptionCode == EXCEPTION_BREAKPOINT && (context->Eip == (DWORD)code_mem + 0xa))
        context->Eip += 1;
    return ExceptionContinueExecution;
}


static const BYTE call_one_arg_code[] = {
        0x8b, 0x44, 0x24, 0x08, /* mov 0x8(%esp),%eax */
        0x50,                   /* push %eax */
        0x8b, 0x44, 0x24, 0x08, /* mov 0x8(%esp),%eax */
        0xff, 0xd0,             /* call *%eax */
        0x90,                   /* nop */
        0x90,                   /* nop */
        0x90,                   /* nop */
        0x90,                   /* nop */
        0xc3,                   /* ret */
};


static void run_rtlraiseexception_test(DWORD exceptioncode)
{
    EXCEPTION_REGISTRATION_RECORD frame;
    EXCEPTION_RECORD record;
    PVOID vectored_handler = NULL;

    void (*func)(void* function, EXCEPTION_RECORD* record) = code_mem;

    record.ExceptionCode = exceptioncode;
    record.ExceptionFlags = 0;
    record.ExceptionRecord = NULL;
    record.ExceptionAddress = NULL; /* does not matter, copied return address */
    record.NumberParameters = 0;

    frame.Handler = rtlraiseexception_handler;
    frame.Prev = NtCurrentTeb()->Tib.ExceptionList;

    memcpy(code_mem, call_one_arg_code, sizeof(call_one_arg_code));

    NtCurrentTeb()->Tib.ExceptionList = &frame;
    if (have_vectored_api)
    {
        vectored_handler = pRtlAddVectoredExceptionHandler(TRUE, &rtlraiseexception_vectored_handler);
        ok(vectored_handler != 0, "RtlAddVectoredExceptionHandler failed\n");
    }

    func(pRtlRaiseException, &record);
    ok( record.ExceptionAddress == (char *)code_mem + 0x0b,
        "address set to %p instead of %p\n", record.ExceptionAddress, (char *)code_mem + 0x0b );

    if (have_vectored_api)
        pRtlRemoveVectoredExceptionHandler(vectored_handler);
    NtCurrentTeb()->Tib.ExceptionList = frame.Prev;
}

static void test_rtlraiseexception(void)
{
    if (!pRtlRaiseException)
    {
        skip("RtlRaiseException not found\n");
        return;
    }

    /* test without debugger */
    run_rtlraiseexception_test(0x12345);
    run_rtlraiseexception_test(EXCEPTION_BREAKPOINT);
    run_rtlraiseexception_test(EXCEPTION_INVALID_HANDLE);
}

static DWORD unwind_expected_eax;

static DWORD unwind_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                             CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    trace("exception: %08x flags:%x addr:%p context: Eip:%x\n",
          rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress, context->Eip);

    ok(rec->ExceptionCode == STATUS_UNWIND, "ExceptionCode is %08x instead of %08x\n",
       rec->ExceptionCode, STATUS_UNWIND);
    ok(rec->ExceptionAddress == (char *)code_mem + 0x22 || broken(TRUE) /* Win10 1709 */,
       "ExceptionAddress at %p instead of %p\n", rec->ExceptionAddress, (char *)code_mem + 0x22);
    ok(context->Eip == (DWORD)code_mem + 0x22, "context->Eip is %08x instead of %08x\n",
       context->Eip, (DWORD)code_mem + 0x22);
    ok(context->Eax == unwind_expected_eax, "context->Eax is %08x instead of %08x\n",
       context->Eax, unwind_expected_eax);

    context->Eax += 1;
    return ExceptionContinueSearch;
}

static const BYTE call_unwind_code[] = {
    0x55,                           /* push %ebp */
    0x53,                           /* push %ebx */
    0x56,                           /* push %esi */
    0x57,                           /* push %edi */
    0xe8, 0x00, 0x00, 0x00, 0x00,   /* call 0 */
    0x58,                           /* 0: pop %eax */
    0x05, 0x1e, 0x00, 0x00, 0x00,   /* add $0x1e,%eax */
    0xff, 0x74, 0x24, 0x20,         /* push 0x20(%esp) */
    0xff, 0x74, 0x24, 0x20,         /* push 0x20(%esp) */
    0x50,                           /* push %eax */
    0xff, 0x74, 0x24, 0x24,         /* push 0x24(%esp) */
    0x8B, 0x44, 0x24, 0x24,         /* mov 0x24(%esp),%eax */
    0xff, 0xd0,                     /* call *%eax */
    0x5f,                           /* pop %edi */
    0x5e,                           /* pop %esi */
    0x5b,                           /* pop %ebx */
    0x5d,                           /* pop %ebp */
    0xc3,                           /* ret */
    0xcc,                           /* int $3 */
};

static void test_unwind(void)
{
    EXCEPTION_REGISTRATION_RECORD frames[2], *frame2 = &frames[0], *frame1 = &frames[1];
    DWORD (*func)(void* function, EXCEPTION_REGISTRATION_RECORD *pEndFrame, EXCEPTION_RECORD* record, DWORD retval) = code_mem;
    DWORD retval;

    memcpy(code_mem, call_unwind_code, sizeof(call_unwind_code));

    /* add first unwind handler */
    frame1->Handler = unwind_handler;
    frame1->Prev = NtCurrentTeb()->Tib.ExceptionList;
    NtCurrentTeb()->Tib.ExceptionList = frame1;

    /* add second unwind handler */
    frame2->Handler = unwind_handler;
    frame2->Prev = NtCurrentTeb()->Tib.ExceptionList;
    NtCurrentTeb()->Tib.ExceptionList = frame2;

    /* test unwind to current frame */
    unwind_expected_eax = 0xDEAD0000;
    retval = func(pRtlUnwind, frame2, NULL, 0xDEAD0000);
    ok(retval == 0xDEAD0000, "RtlUnwind returned eax %08x instead of %08x\n", retval, 0xDEAD0000);
    ok(NtCurrentTeb()->Tib.ExceptionList == frame2, "Exception record points to %p instead of %p\n",
       NtCurrentTeb()->Tib.ExceptionList, frame2);

    /* unwind to frame1 */
    unwind_expected_eax = 0xDEAD0000;
    retval = func(pRtlUnwind, frame1, NULL, 0xDEAD0000);
    ok(retval == 0xDEAD0001, "RtlUnwind returned eax %08x instead of %08x\n", retval, 0xDEAD0001);
    ok(NtCurrentTeb()->Tib.ExceptionList == frame1, "Exception record points to %p instead of %p\n",
       NtCurrentTeb()->Tib.ExceptionList, frame1);

    /* restore original handler */
    NtCurrentTeb()->Tib.ExceptionList = frame1->Prev;
}

static DWORD handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                      CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    const struct exception *except = *(const struct exception **)(frame + 1);
    unsigned int i, parameter_count, entry = except - exceptions;

    got_exception++;
    trace( "exception %u: %x flags:%x addr:%p\n",
           entry, rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress );

    ok( rec->ExceptionCode == except->status ||
        (except->alt_status != 0 && rec->ExceptionCode == except->alt_status),
        "%u: Wrong exception code %x/%x\n", entry, rec->ExceptionCode, except->status );
    ok( context->Eip == (DWORD_PTR)code_mem + except->offset,
        "%u: Unexpected eip %#x/%#lx\n", entry,
        context->Eip, (DWORD_PTR)code_mem + except->offset );
    ok( rec->ExceptionAddress == (char*)context->Eip ||
        (rec->ExceptionCode == STATUS_BREAKPOINT && rec->ExceptionAddress == (char*)context->Eip + 1),
        "%u: Unexpected exception address %p/%p\n", entry,
        rec->ExceptionAddress, (char*)context->Eip );

    if (except->status == STATUS_BREAKPOINT && is_wow64)
        parameter_count = 1;
    else if (except->alt_status == 0 || rec->ExceptionCode != except->alt_status)
        parameter_count = except->nb_params;
    else
        parameter_count = except->alt_nb_params;

    ok( rec->NumberParameters == parameter_count,
        "%u: Unexpected parameter count %u/%u\n", entry, rec->NumberParameters, parameter_count );

    /* Most CPUs (except Intel Core apparently) report a segment limit violation */
    /* instead of page faults for accesses beyond 0xffffffff */
    if (except->nb_params == 2 && except->params[1] >= 0xfffffffd)
    {
        if (rec->ExceptionInformation[0] == 0 && rec->ExceptionInformation[1] == 0xffffffff)
            goto skip_params;
    }

    /* Seems that both 0xbee8 and 0xfffffffff can be returned in windows */
    if (except->nb_params == 2 && rec->NumberParameters == 2
        && except->params[1] == 0xbee8 && rec->ExceptionInformation[1] == 0xffffffff
        && except->params[0] == rec->ExceptionInformation[0])
    {
        goto skip_params;
    }

    if (except->alt_status == 0 || rec->ExceptionCode != except->alt_status)
    {
        for (i = 0; i < rec->NumberParameters; i++)
            ok( rec->ExceptionInformation[i] == except->params[i],
                "%u: Wrong parameter %d: %lx/%x\n",
                entry, i, rec->ExceptionInformation[i], except->params[i] );
    }
    else
    {
        for (i = 0; i < rec->NumberParameters; i++)
            ok( rec->ExceptionInformation[i] == except->alt_params[i],
                "%u: Wrong parameter %d: %lx/%x\n",
                entry, i, rec->ExceptionInformation[i], except->alt_params[i] );
    }

skip_params:
    /* don't handle exception if it's not the address we expected */
    if (context->Eip != (DWORD_PTR)code_mem + except->offset) return ExceptionContinueSearch;

    context->Eip += except->length;
    return ExceptionContinueExecution;
}

static void test_prot_fault(void)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(exceptions); i++)
    {
        if (is_wow64 && exceptions[i].wow64_broken && !strcmp( winetest_platform, "windows" ))
        {
            skip( "Exception %u broken on Wow64\n", i );
            continue;
        }
        got_exception = 0;
        run_exception_test(handler, &exceptions[i], &exceptions[i].code,
                           sizeof(exceptions[i].code), 0);
        if (!i && !got_exception)
        {
            trace( "No exception, assuming win9x, no point in testing further\n" );
            break;
        }
        ok( got_exception == (exceptions[i].status != 0),
            "%u: bad exception count %d\n", i, got_exception );
    }
}

struct dbgreg_test {
    DWORD dr0, dr1, dr2, dr3, dr6, dr7;
};

/* test handling of debug registers */
static DWORD dreg_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                      CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    const struct dbgreg_test *test = *(const struct dbgreg_test **)(frame + 1);

    context->Eip += 2;	/* Skips the popl (%eax) */
    context->Dr0 = test->dr0;
    context->Dr1 = test->dr1;
    context->Dr2 = test->dr2;
    context->Dr3 = test->dr3;
    context->Dr6 = test->dr6;
    context->Dr7 = test->dr7;
    return ExceptionContinueExecution;
}

#define CHECK_DEBUG_REG(n, m) \
    ok((ctx.Dr##n & m) == test->dr##n, "(%d) failed to set debug register " #n " to %x, got %x\n", \
       test_num, test->dr##n, ctx.Dr##n)

static void check_debug_registers(int test_num, const struct dbgreg_test *test)
{
    CONTEXT ctx;
    NTSTATUS status;

    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    status = pNtGetContextThread(GetCurrentThread(), &ctx);
    ok(status == STATUS_SUCCESS, "NtGetContextThread failed with %x\n", status);

    CHECK_DEBUG_REG(0, ~0);
    CHECK_DEBUG_REG(1, ~0);
    CHECK_DEBUG_REG(2, ~0);
    CHECK_DEBUG_REG(3, ~0);
    CHECK_DEBUG_REG(6, 0x0f);
    CHECK_DEBUG_REG(7, ~0xdc00);
}

static const BYTE segfault_code[5] = {
	0x31, 0xc0, /* xor    %eax,%eax */
	0x8f, 0x00, /* popl   (%eax) - cause exception */
        0xc3        /* ret */
};

/* test the single step exception behaviour */
static DWORD single_step_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                  CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    got_exception++;
    ok (!(context->EFlags & 0x100), "eflags has single stepping bit set\n");

    if( got_exception < 3)
        context->EFlags |= 0x100;  /* single step until popf instruction */
    else {
        /* show that the last single step exception on the popf instruction
         * (which removed the TF bit), still is a EXCEPTION_SINGLE_STEP exception */
        ok( rec->ExceptionCode == EXCEPTION_SINGLE_STEP,
            "exception is not EXCEPTION_SINGLE_STEP: %x\n", rec->ExceptionCode);
    }

    return ExceptionContinueExecution;
}

static const BYTE single_stepcode[] = {
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

/* Test the alignment check (AC) flag handling. */
static const BYTE align_check_code[] = {
    0x55,                  	/* push   %ebp */
    0x89,0xe5,             	/* mov    %esp,%ebp */
    0x9c,                  	/* pushf   */
    0x9c,                  	/* pushf   */
    0x58,                  	/* pop    %eax */
    0x0d,0,0,4,0,       	/* or     $0x40000,%eax */
    0x50,                  	/* push   %eax */
    0x9d,                  	/* popf    */
    0x89,0xe0,                  /* mov %esp, %eax */
    0x8b,0x40,0x1,              /* mov 0x1(%eax), %eax - cause exception */
    0x9d,                  	/* popf    */
    0x5d,                  	/* pop    %ebp */
    0xc3,                  	/* ret     */
};

static DWORD align_check_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                  CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    ok (!(context->EFlags & 0x40000), "eflags has AC bit set\n");
    got_exception++;
    return ExceptionContinueExecution;
}

/* Test the direction flag handling. */
static const BYTE direction_flag_code[] = {
    0x55,                  	/* push   %ebp */
    0x89,0xe5,             	/* mov    %esp,%ebp */
    0xfd,                  	/* std */
    0xfa,                  	/* cli - cause exception */
    0x5d,                  	/* pop    %ebp */
    0xc3,                  	/* ret     */
};

static DWORD direction_flag_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                     CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
#ifdef __GNUC__
    unsigned int flags;
    __asm__("pushfl; popl %0; cld" : "=r" (flags) );
    /* older windows versions don't clear DF properly so don't test */
    if (flags & 0x400) trace( "eflags has DF bit set\n" );
#endif
    ok( context->EFlags & 0x400, "context eflags has DF bit cleared\n" );
    got_exception++;
    context->Eip++;  /* skip cli */
    context->EFlags &= ~0x400;  /* make sure it is cleared on return */
    return ExceptionContinueExecution;
}

/* test single stepping over hardware breakpoint */
static DWORD bpx_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                          CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    got_exception++;
    ok( rec->ExceptionCode == EXCEPTION_SINGLE_STEP,
        "wrong exception code: %x\n", rec->ExceptionCode);

    if(got_exception == 1) {
        /* hw bp exception on first nop */
        ok( context->Eip == (DWORD)code_mem, "eip is wrong:  %x instead of %x\n",
                                             context->Eip, (DWORD)code_mem);
        ok( (context->Dr6 & 0xf) == 1, "B0 flag is not set in Dr6\n");
        ok( !(context->Dr6 & 0x4000), "BS flag is set in Dr6\n");
        context->Dr0 = context->Dr0 + 1;  /* set hw bp again on next instruction */
        context->EFlags |= 0x100;       /* enable single stepping */
    } else if(  got_exception == 2) {
        /* single step exception on second nop */
        ok( context->Eip == (DWORD)code_mem + 1, "eip is wrong: %x instead of %x\n",
                                                 context->Eip, (DWORD)code_mem + 1);
        ok( (context->Dr6 & 0x4000), "BS flag is not set in Dr6\n");
       /* depending on the win version the B0 bit is already set here as well
        ok( (context->Dr6 & 0xf) == 0, "B0...3 flags in Dr6 shouldn't be set\n"); */
        context->EFlags |= 0x100;
    } else if( got_exception == 3) {
        /* hw bp exception on second nop */
        ok( context->Eip == (DWORD)code_mem + 1, "eip is wrong: %x instead of %x\n",
                                                 context->Eip, (DWORD)code_mem + 1);
        ok( (context->Dr6 & 0xf) == 1, "B0 flag is not set in Dr6\n");
        ok( !(context->Dr6 & 0x4000), "BS flag is set in Dr6\n");
        context->Dr0 = 0;       /* clear breakpoint */
        context->EFlags |= 0x100;
    } else {
        /* single step exception on ret */
        ok( context->Eip == (DWORD)code_mem + 2, "eip is wrong: %x instead of %x\n",
                                                 context->Eip, (DWORD)code_mem + 2);
        ok( (context->Dr6 & 0xf) == 0, "B0...3 flags in Dr6 shouldn't be set\n");
        ok( (context->Dr6 & 0x4000), "BS flag is not set in Dr6\n");
    }

    context->Dr6 = 0;  /* clear status register */
    return ExceptionContinueExecution;
}

static const BYTE dummy_code[] = { 0x90, 0x90, 0xc3 };  /* nop, nop, ret */

/* test int3 handling */
static DWORD int3_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                           CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    ok( rec->ExceptionAddress == code_mem, "exception address not at: %p, but at %p\n",
                                           code_mem,  rec->ExceptionAddress);
    ok( context->Eip == (DWORD)code_mem, "eip not at: %p, but at %#x\n", code_mem, context->Eip);
    if(context->Eip == (DWORD)code_mem) context->Eip++; /* skip breakpoint */

    return ExceptionContinueExecution;
}

static const BYTE int3_code[] = { 0xCC, 0xc3 };  /* int 3, ret */

static DWORD WINAPI hw_reg_exception_thread( void *arg )
{
    int expect = (ULONG_PTR)arg;
    got_exception = 0;
    run_exception_test( bpx_handler, NULL, dummy_code, sizeof(dummy_code), 0 );
    ok( got_exception == expect, "expected %u exceptions, got %d\n", expect, got_exception );
    return 0;
}

static void test_exceptions(void)
{
    CONTEXT ctx;
    NTSTATUS res;
    struct dbgreg_test dreg_test;
    HANDLE h;

    if (!pNtGetContextThread || !pNtSetContextThread)
    {
        skip( "NtGetContextThread/NtSetContextThread not found\n" );
        return;
    }

    /* test handling of debug registers */
    memset(&dreg_test, 0, sizeof(dreg_test));

    dreg_test.dr0 = 0x42424240;
    dreg_test.dr2 = 0x126bb070;
    dreg_test.dr3 = 0x0badbad0;
    dreg_test.dr7 = 0xffff0115;
    run_exception_test(dreg_handler, &dreg_test, &segfault_code, sizeof(segfault_code), 0);
    check_debug_registers(1, &dreg_test);

    dreg_test.dr0 = 0x42424242;
    dreg_test.dr2 = 0x100f0fe7;
    dreg_test.dr3 = 0x0abebabe;
    dreg_test.dr7 = 0x115;
    run_exception_test(dreg_handler, &dreg_test, &segfault_code, sizeof(segfault_code), 0);
    check_debug_registers(2, &dreg_test);

    /* test single stepping behavior */
    got_exception = 0;
    run_exception_test(single_step_handler, NULL, &single_stepcode, sizeof(single_stepcode), 0);
    ok(got_exception == 3, "expected 3 single step exceptions, got %d\n", got_exception);

    /* test alignment exceptions */
    got_exception = 0;
    run_exception_test(align_check_handler, NULL, align_check_code, sizeof(align_check_code), 0);
    ok(got_exception == 0, "got %d alignment faults, expected 0\n", got_exception);

    /* test direction flag */
    got_exception = 0;
    run_exception_test(direction_flag_handler, NULL, direction_flag_code, sizeof(direction_flag_code), 0);
    ok(got_exception == 1, "got %d exceptions, expected 1\n", got_exception);

    /* test single stepping over hardware breakpoint */
    memset(&ctx, 0, sizeof(ctx));
    ctx.Dr0 = (DWORD) code_mem;  /* set hw bp on first nop */
    ctx.Dr7 = 3;
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    res = pNtSetContextThread( GetCurrentThread(), &ctx);
    ok( res == STATUS_SUCCESS, "NtSetContextThread failed with %x\n", res);

    got_exception = 0;
    run_exception_test(bpx_handler, NULL, dummy_code, sizeof(dummy_code), 0);
    ok( got_exception == 4,"expected 4 exceptions, got %d\n", got_exception);

    /* test int3 handling */
    run_exception_test(int3_handler, NULL, int3_code, sizeof(int3_code), 0);

    /* test that hardware breakpoints are not inherited by created threads */
    res = pNtSetContextThread( GetCurrentThread(), &ctx );
    ok( res == STATUS_SUCCESS, "NtSetContextThread failed with %x\n", res );

    h = CreateThread( NULL, 0, hw_reg_exception_thread, 0, 0, NULL );
    WaitForSingleObject( h, 10000 );
    CloseHandle( h );

    h = CreateThread( NULL, 0, hw_reg_exception_thread, (void *)4, CREATE_SUSPENDED, NULL );
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    res = pNtGetContextThread( h, &ctx );
    ok( res == STATUS_SUCCESS, "NtGetContextThread failed with %x\n", res );
    ok( ctx.Dr0 == 0, "dr0 %x\n", ctx.Dr0 );
    ok( ctx.Dr7 == 0, "dr7 %x\n", ctx.Dr7 );
    ctx.Dr0 = (DWORD)code_mem;
    ctx.Dr7 = 3;
    res = pNtSetContextThread( h, &ctx );
    ok( res == STATUS_SUCCESS, "NtSetContextThread failed with %x\n", res );
    ResumeThread( h );
    WaitForSingleObject( h, 10000 );
    CloseHandle( h );

    ctx.Dr0 = 0;
    ctx.Dr7 = 0;
    res = pNtSetContextThread( GetCurrentThread(), &ctx );
    ok( res == STATUS_SUCCESS, "NtSetContextThread failed with %x\n", res );
}

static void test_debugger(DWORD cont_status)
{
    char cmdline[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { 0 };
    DEBUG_EVENT de;
    DWORD continuestatus;
    PVOID code_mem_address = NULL;
    NTSTATUS status;
    SIZE_T size_read;
    BOOL ret;
    int counter = 0;
    si.cb = sizeof(si);

    if(!pNtGetContextThread || !pNtSetContextThread || !pNtReadVirtualMemory || !pNtTerminateProcess)
    {
        skip("NtGetContextThread, NtSetContextThread, NtReadVirtualMemory or NtTerminateProcess not found\n");
        return;
    }

    sprintf(cmdline, "%s %s %s %p", my_argv[0], my_argv[1], "debuggee", &test_stage);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, NULL, &si, &pi);
    ok(ret, "could not create child process error: %u\n", GetLastError());
    if (!ret)
        return;

    do
    {
        continuestatus = cont_status;
        ok(WaitForDebugEvent(&de, INFINITE), "reading debug event\n");

        ret = ContinueDebugEvent(de.dwProcessId, de.dwThreadId, 0xdeadbeef);
        ok(!ret, "ContinueDebugEvent unexpectedly succeeded\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "Unexpected last error: %u\n", GetLastError());

        if (de.dwThreadId != pi.dwThreadId)
        {
            trace("event %d not coming from main thread, ignoring\n", de.dwDebugEventCode);
            ContinueDebugEvent(de.dwProcessId, de.dwThreadId, cont_status);
            continue;
        }

        if (de.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
        {
            if(de.u.CreateProcessInfo.lpBaseOfImage != NtCurrentTeb()->Peb->ImageBaseAddress)
            {
                skip("child process loaded at different address, terminating it\n");
                pNtTerminateProcess(pi.hProcess, 0);
            }
        }
        else if (de.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
        {
            CONTEXT ctx;
            int stage;

            counter++;
            status = pNtReadVirtualMemory(pi.hProcess, &code_mem, &code_mem_address,
                                          sizeof(code_mem_address), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);
            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);

            ctx.ContextFlags = CONTEXT_FULL | CONTEXT_EXTENDED_REGISTERS;
            status = pNtGetContextThread(pi.hThread, &ctx);
            ok(!status, "NtGetContextThread failed with 0x%x\n", status);

            trace("exception 0x%x at %p firstchance=%d Eip=0x%x, Eax=0x%x\n",
                  de.u.Exception.ExceptionRecord.ExceptionCode,
                  de.u.Exception.ExceptionRecord.ExceptionAddress, de.u.Exception.dwFirstChance, ctx.Eip, ctx.Eax);

            if (counter > 100)
            {
                ok(FALSE, "got way too many exceptions, probably caught in an infinite loop, terminating child\n");
                pNtTerminateProcess(pi.hProcess, 1);
            }
            else if (counter < 2) /* startup breakpoint */
            {
                /* breakpoint is inside ntdll */
                void *ntdll = GetModuleHandleA( "ntdll.dll" );
                IMAGE_NT_HEADERS *nt = RtlImageNtHeader( ntdll );

                ok( (char *)ctx.Eip >= (char *)ntdll &&
                    (char *)ctx.Eip < (char *)ntdll + nt->OptionalHeader.SizeOfImage,
                    "wrong eip %p ntdll %p-%p\n", (void *)ctx.Eip, ntdll,
                    (char *)ntdll + nt->OptionalHeader.SizeOfImage );
            }
            else
            {
                if (stage == 1)
                {
                    ok((char *)ctx.Eip == (char *)code_mem_address + 0xb, "Eip at %x instead of %p\n",
                       ctx.Eip, (char *)code_mem_address + 0xb);
                    /* setting the context from debugger does not affect the context that the
                     * exception handler gets, except on w2008 */
                    ctx.Eip = (UINT_PTR)code_mem_address + 0xd;
                    ctx.Eax = 0xf00f00f1;
                    /* let the debuggee handle the exception */
                    continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == 2)
                {
                    if (de.u.Exception.dwFirstChance)
                    {
                        /* debugger gets first chance exception with unmodified ctx.Eip */
                        ok((char *)ctx.Eip == (char *)code_mem_address + 0xb, "Eip at 0x%x instead of %p\n",
                           ctx.Eip, (char *)code_mem_address + 0xb);
                        ctx.Eip = (UINT_PTR)code_mem_address + 0xd;
                        ctx.Eax = 0xf00f00f1;
                        /* pass exception to debuggee
                         * exception will not be handled and a second chance exception will be raised */
                        continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                    }
                    else
                    {
                        /* debugger gets context after exception handler has played with it */
                        /* ctx.Eip is the same value the exception handler got */
                        if (de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
                        {
                            ok((char *)ctx.Eip == (char *)code_mem_address + 0xa ||
                               broken(is_wow64 && (char *)ctx.Eip == (char *)code_mem_address + 0xb) ||
                               broken((char *)ctx.Eip == (char *)code_mem_address + 0xd) /* w2008 */,
                               "Eip at 0x%x instead of %p\n",
                                ctx.Eip, (char *)code_mem_address + 0xa);
                            /* need to fixup Eip for debuggee */
                            if ((char *)ctx.Eip == (char *)code_mem_address + 0xa)
                                ctx.Eip += 1;
                        }
                        else
                            ok((char *)ctx.Eip == (char *)code_mem_address + 0xb ||
                               broken((char *)ctx.Eip == (char *)code_mem_address + 0xd) /* w2008 */,
                               "Eip at 0x%x instead of %p\n",
                               ctx.Eip, (char *)code_mem_address + 0xb);
                        /* here we handle exception */
                    }
                }
                else if (stage == 7 || stage == 8)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT,
                       "expected EXCEPTION_BREAKPOINT, got %08x\n", de.u.Exception.ExceptionRecord.ExceptionCode);
                    ok((char *)ctx.Eip == (char *)code_mem_address + 0x1d,
                       "expected Eip = %p, got 0x%x\n", (char *)code_mem_address + 0x1d, ctx.Eip);

                    if (stage == 8) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == 9 || stage == 10)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT,
                       "expected EXCEPTION_BREAKPOINT, got %08x\n", de.u.Exception.ExceptionRecord.ExceptionCode);
                    ok((char *)ctx.Eip == (char *)code_mem_address + 2,
                       "expected Eip = %p, got 0x%x\n", (char *)code_mem_address + 2, ctx.Eip);

                    if (stage == 10) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == 11 || stage == 12 || stage == 13)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_INVALID_HANDLE,
                       "unexpected exception code %08x, expected %08x\n", de.u.Exception.ExceptionRecord.ExceptionCode,
                       EXCEPTION_INVALID_HANDLE);
                    ok(de.u.Exception.ExceptionRecord.NumberParameters == 0,
                       "unexpected number of parameters %d, expected 0\n", de.u.Exception.ExceptionRecord.NumberParameters);

                    if (stage == 12|| stage == 13) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == 14 || stage == 15)
                {
                    test_debugger_xstate(pi.hThread, &ctx, stage);
                }
                else if (stage == 16)
                {
                    USHORT ss;
                    __asm__( "movw %%ss,%0" : "=r" (ss) );
                    ok( ctx.SegSs == ss, "wrong ss %04x / %04x\n", ctx.SegSs, ss );
                    ok( ctx.SegFs != ctx.SegSs, "wrong fs %04x / %04x\n", ctx.SegFs, ctx.SegSs );
                    if (is_wow64)
                    {
                        ok( ctx.SegDs == ctx.SegSs, "wrong ds %04x / %04x\n", ctx.SegDs, ctx.SegSs );
                        ok( ctx.SegEs == ctx.SegSs, "wrong es %04x / %04x\n", ctx.SegEs, ctx.SegSs );
                        ok( ctx.SegGs == ctx.SegSs, "wrong gs %04x / %04x\n", ctx.SegGs, ctx.SegSs );
                    }
                    else
                    {
                        ok( !ctx.SegDs, "wrong ds %04x / %04x\n", ctx.SegDs, ctx.SegSs );
                        ok( !ctx.SegEs, "wrong es %04x / %04x\n", ctx.SegEs, ctx.SegSs );
                        ok( !ctx.SegGs, "wrong gs %04x / %04x\n", ctx.SegGs, ctx.SegSs );
                    }
                }
                else
                    ok(FALSE, "unexpected stage %u\n", stage);

                status = pNtSetContextThread(pi.hThread, &ctx);
                ok(!status, "NtSetContextThread failed with 0x%x\n", status);
            }
        }
        else if (de.dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT)
        {
            int stage;
            char buffer[64];

            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);

            ok(!de.u.DebugString.fUnicode, "unexpected unicode debug string event\n");
            ok(de.u.DebugString.nDebugStringLength < sizeof(buffer) - 1, "buffer not large enough to hold %d bytes\n",
               de.u.DebugString.nDebugStringLength);

            memset(buffer, 0, sizeof(buffer));
            status = pNtReadVirtualMemory(pi.hProcess, de.u.DebugString.lpDebugStringData, buffer,
                                          de.u.DebugString.nDebugStringLength, &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);

            if (stage == 3 || stage == 4)
                ok(!strcmp(buffer, "Hello World"), "got unexpected debug string '%s'\n", buffer);
            else /* ignore unrelated debug strings like 'SHIMVIEW: ShimInfo(Complete)' */
                ok(strstr(buffer, "SHIMVIEW") != NULL, "unexpected stage %x, got debug string event '%s'\n", stage, buffer);

            if (stage == 4) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
        }
        else if (de.dwDebugEventCode == RIP_EVENT)
        {
            int stage;

            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);

            if (stage == 5 || stage == 6)
            {
                ok(de.u.RipInfo.dwError == 0x11223344, "got unexpected rip error code %08x, expected %08x\n",
                   de.u.RipInfo.dwError, 0x11223344);
                ok(de.u.RipInfo.dwType  == 0x55667788, "got unexpected rip type %08x, expected %08x\n",
                   de.u.RipInfo.dwType, 0x55667788);
            }
            else
                ok(FALSE, "unexpected stage %x\n", stage);

            if (stage == 6) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
        }

        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, continuestatus);

    } while (de.dwDebugEventCode != EXIT_PROCESS_DEBUG_EVENT);

    wait_child_process( pi.hProcess );
    ret = CloseHandle(pi.hThread);
    ok(ret, "error %u\n", GetLastError());
    ret = CloseHandle(pi.hProcess);
    ok(ret, "error %u\n", GetLastError());

    return;
}

static DWORD simd_fault_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                 CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    int *stage = *(int **)(frame + 1);

    got_exception++;

    if( *stage == 1) {
        /* fault while executing sse instruction */
        context->Eip += 3; /* skip addps */
        return ExceptionContinueExecution;
    }
    else if ( *stage == 2 || *stage == 3 ) {
        /* stage 2 - divide by zero fault */
        /* stage 3 - invalid operation fault */
        if( rec->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION)
            skip("system doesn't support SIMD exceptions\n");
        else {
            ok( rec->ExceptionCode ==  STATUS_FLOAT_MULTIPLE_TRAPS,
                "exception code: %#x, should be %#x\n",
                rec->ExceptionCode,  STATUS_FLOAT_MULTIPLE_TRAPS);
            ok( rec->NumberParameters == is_wow64 ? 2 : 1, "# of params: %i\n", rec->NumberParameters);
            if (rec->NumberParameters >= 1)
                ok( rec->ExceptionInformation[0] == 0, "param #1: %lx, should be 0\n", rec->ExceptionInformation[0]);
        }
        context->Eip += 3; /* skip divps */
    }
    else
        ok(FALSE, "unexpected stage %x\n", *stage);

    return ExceptionContinueExecution;
}

static const BYTE simd_exception_test[] = {
    0x83, 0xec, 0x4,                     /* sub $0x4, %esp       */
    0x0f, 0xae, 0x1c, 0x24,              /* stmxcsr (%esp)       */
    0x8b, 0x04, 0x24,                    /* mov    (%esp),%eax   * store mxcsr */
    0x66, 0x81, 0x24, 0x24, 0xff, 0xfd,  /* andw $0xfdff,(%esp)  * enable divide by */
    0x0f, 0xae, 0x14, 0x24,              /* ldmxcsr (%esp)       * zero exceptions  */
    0x6a, 0x01,                          /* push   $0x1          */
    0x6a, 0x01,                          /* push   $0x1          */
    0x6a, 0x01,                          /* push   $0x1          */
    0x6a, 0x01,                          /* push   $0x1          */
    0x0f, 0x10, 0x0c, 0x24,              /* movups (%esp),%xmm1  * fill dividend  */
    0x0f, 0x57, 0xc0,                    /* xorps  %xmm0,%xmm0   * clear divisor  */
    0x0f, 0x5e, 0xc8,                    /* divps  %xmm0,%xmm1   * generate fault */
    0x83, 0xc4, 0x10,                    /* add    $0x10,%esp    */
    0x89, 0x04, 0x24,                    /* mov    %eax,(%esp)   * restore to old mxcsr */
    0x0f, 0xae, 0x14, 0x24,              /* ldmxcsr (%esp)       */
    0x83, 0xc4, 0x04,                    /* add    $0x4,%esp     */
    0xc3,                                /* ret */
};

static const BYTE simd_exception_test2[] = {
    0x83, 0xec, 0x4,                     /* sub $0x4, %esp       */
    0x0f, 0xae, 0x1c, 0x24,              /* stmxcsr (%esp)       */
    0x8b, 0x04, 0x24,                    /* mov    (%esp),%eax   * store mxcsr */
    0x66, 0x81, 0x24, 0x24, 0x7f, 0xff,  /* andw $0xff7f,(%esp)  * enable invalid       */
    0x0f, 0xae, 0x14, 0x24,              /* ldmxcsr (%esp)       * operation exceptions */
    0x0f, 0x57, 0xc9,                    /* xorps  %xmm1,%xmm1   * clear dividend */
    0x0f, 0x57, 0xc0,                    /* xorps  %xmm0,%xmm0   * clear divisor  */
    0x0f, 0x5e, 0xc8,                    /* divps  %xmm0,%xmm1   * generate fault */
    0x89, 0x04, 0x24,                    /* mov    %eax,(%esp)   * restore to old mxcsr */
    0x0f, 0xae, 0x14, 0x24,              /* ldmxcsr (%esp)       */
    0x83, 0xc4, 0x04,                    /* add    $0x4,%esp     */
    0xc3,                                /* ret */
};

static const BYTE sse_check[] = {
    0x0f, 0x58, 0xc8,                    /* addps  %xmm0,%xmm1 */
    0xc3,                                /* ret */
};

static void test_simd_exceptions(void)
{
    int stage;

    /* test if CPU & OS can do sse */
    stage = 1;
    got_exception = 0;
    run_exception_test(simd_fault_handler, &stage, sse_check, sizeof(sse_check), 0);
    if(got_exception) {
        skip("system doesn't support SSE\n");
        return;
    }

    /* generate a SIMD exception */
    stage = 2;
    got_exception = 0;
    run_exception_test(simd_fault_handler, &stage, simd_exception_test,
                       sizeof(simd_exception_test), 0);
    ok(got_exception == 1, "got exception: %i, should be 1\n", got_exception);

    /* generate a SIMD exception, test FPE_FLTINV */
    stage = 3;
    got_exception = 0;
    run_exception_test(simd_fault_handler, &stage, simd_exception_test2,
                       sizeof(simd_exception_test2), 0);
    ok(got_exception == 1, "got exception: %i, should be 1\n", got_exception);
}

struct fpu_exception_info
{
    DWORD exception_code;
    DWORD exception_offset;
    DWORD eip_offset;
};

static DWORD fpu_exception_handler(EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
        CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher)
{
    struct fpu_exception_info *info = *(struct fpu_exception_info **)(frame + 1);

    info->exception_code = rec->ExceptionCode;
    info->exception_offset = (BYTE *)rec->ExceptionAddress - (BYTE *)code_mem;
    info->eip_offset = context->Eip - (DWORD)code_mem;

    ++context->Eip;
    return ExceptionContinueExecution;
}

static void test_fpu_exceptions(void)
{
    static const BYTE fpu_exception_test_ie[] =
    {
        0x83, 0xec, 0x04,                   /* sub $0x4,%esp        */
        0x66, 0xc7, 0x04, 0x24, 0xfe, 0x03, /* movw $0x3fe,(%esp)   */
        0x9b, 0xd9, 0x7c, 0x24, 0x02,       /* fstcw 0x2(%esp)      */
        0xd9, 0x2c, 0x24,                   /* fldcw (%esp)         */
        0xd9, 0xee,                         /* fldz                 */
        0xd9, 0xe8,                         /* fld1                 */
        0xde, 0xf1,                         /* fdivp                */
        0xdd, 0xd8,                         /* fstp %st(0)          */
        0xdd, 0xd8,                         /* fstp %st(0)          */
        0x9b,                               /* fwait                */
        0xdb, 0xe2,                         /* fnclex               */
        0xd9, 0x6c, 0x24, 0x02,             /* fldcw 0x2(%esp)      */
        0x83, 0xc4, 0x04,                   /* add $0x4,%esp        */
        0xc3,                               /* ret                  */
    };

    static const BYTE fpu_exception_test_de[] =
    {
        0x83, 0xec, 0x04,                   /* sub $0x4,%esp        */
        0x66, 0xc7, 0x04, 0x24, 0xfb, 0x03, /* movw $0x3fb,(%esp)   */
        0x9b, 0xd9, 0x7c, 0x24, 0x02,       /* fstcw 0x2(%esp)      */
        0xd9, 0x2c, 0x24,                   /* fldcw (%esp)         */
        0xdd, 0xd8,                         /* fstp %st(0)          */
        0xd9, 0xee,                         /* fldz                 */
        0xd9, 0xe8,                         /* fld1                 */
        0xde, 0xf1,                         /* fdivp                */
        0x9b,                               /* fwait                */
        0xdb, 0xe2,                         /* fnclex               */
        0xdd, 0xd8,                         /* fstp %st(0)          */
        0xdd, 0xd8,                         /* fstp %st(0)          */
        0xd9, 0x6c, 0x24, 0x02,             /* fldcw 0x2(%esp)      */
        0x83, 0xc4, 0x04,                   /* add $0x4,%esp        */
        0xc3,                               /* ret                  */
    };

    struct fpu_exception_info info;

    memset(&info, 0, sizeof(info));
    run_exception_test(fpu_exception_handler, &info, fpu_exception_test_ie, sizeof(fpu_exception_test_ie), 0);
    ok(info.exception_code == EXCEPTION_FLT_STACK_CHECK,
            "Got exception code %#x, expected EXCEPTION_FLT_STACK_CHECK\n", info.exception_code);
    ok(info.exception_offset == 0x19 ||
       broken( info.exception_offset == info.eip_offset ),
       "Got exception offset %#x, expected 0x19\n", info.exception_offset);
    ok(info.eip_offset == 0x1b, "Got EIP offset %#x, expected 0x1b\n", info.eip_offset);

    memset(&info, 0, sizeof(info));
    run_exception_test(fpu_exception_handler, &info, fpu_exception_test_de, sizeof(fpu_exception_test_de), 0);
    ok(info.exception_code == EXCEPTION_FLT_DIVIDE_BY_ZERO,
            "Got exception code %#x, expected EXCEPTION_FLT_DIVIDE_BY_ZERO\n", info.exception_code);
    ok(info.exception_offset == 0x17 ||
       broken( info.exception_offset == info.eip_offset ),
       "Got exception offset %#x, expected 0x17\n", info.exception_offset);
    ok(info.eip_offset == 0x19, "Got EIP offset %#x, expected 0x19\n", info.eip_offset);
}

struct dpe_exception_info {
    BOOL exception_caught;
    DWORD exception_info;
};

static DWORD dpe_exception_handler(EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
        CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher)
{
    DWORD old_prot;
    struct dpe_exception_info *info = *(struct dpe_exception_info **)(frame + 1);

    ok(rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION,
       "Exception code %08x\n", rec->ExceptionCode);
    ok(rec->NumberParameters == 2,
       "Parameter count: %d\n", rec->NumberParameters);
    ok((LPVOID)rec->ExceptionInformation[1] == code_mem,
       "Exception address: %p, expected %p\n",
       (LPVOID)rec->ExceptionInformation[1], code_mem);

    info->exception_info = rec->ExceptionInformation[0];
    info->exception_caught = TRUE;

    VirtualProtect(code_mem, 1, PAGE_EXECUTE_READWRITE, &old_prot);
    return ExceptionContinueExecution;
}

static void test_dpe_exceptions(void)
{
    static const BYTE single_ret[] = {0xC3};
    struct dpe_exception_info info;
    NTSTATUS stat;
    BOOL has_hw_support;
    BOOL is_permanent = FALSE, can_test_without = TRUE, can_test_with = TRUE;
    DWORD val;
    ULONG len;

    /* Query DEP with len too small */
    stat = pNtQueryInformationProcess(GetCurrentProcess(), ProcessExecuteFlags, &val, sizeof val - 1, &len);
    if(stat == STATUS_INVALID_INFO_CLASS)
    {
        skip("This software platform does not support DEP\n");
        return;
    }
    ok(stat == STATUS_INFO_LENGTH_MISMATCH, "buffer too small: %08x\n", stat);

    /* Query DEP */
    stat = pNtQueryInformationProcess(GetCurrentProcess(), ProcessExecuteFlags, &val, sizeof val, &len);
    ok(stat == STATUS_SUCCESS, "querying DEP: status %08x\n", stat);
    if(stat == STATUS_SUCCESS)
    {
        ok(len == sizeof val, "returned length: %d\n", len);
        if(val & MEM_EXECUTE_OPTION_PERMANENT)
        {
            skip("toggling DEP impossible - status locked\n");
            is_permanent = TRUE;
            if(val & MEM_EXECUTE_OPTION_DISABLE)
                can_test_without = FALSE;
            else
                can_test_with = FALSE;
        }
    }

    if(!is_permanent)
    {
        /* Enable DEP */
        val = MEM_EXECUTE_OPTION_DISABLE;
        stat = pNtSetInformationProcess(GetCurrentProcess(), ProcessExecuteFlags, &val, sizeof val);
        ok(stat == STATUS_SUCCESS, "enabling DEP: status %08x\n", stat);
    }

    if(can_test_with)
    {
        /* Try access to locked page with DEP on*/
        info.exception_caught = FALSE;
        run_exception_test(dpe_exception_handler, &info, single_ret, sizeof(single_ret), PAGE_NOACCESS);
        ok(info.exception_caught == TRUE, "Execution of disabled memory succeeded\n");
        ok(info.exception_info == EXCEPTION_READ_FAULT ||
           info.exception_info == EXCEPTION_EXECUTE_FAULT,
              "Access violation type: %08x\n", (unsigned)info.exception_info);
        has_hw_support = info.exception_info == EXCEPTION_EXECUTE_FAULT;
        trace("DEP hardware support: %s\n", has_hw_support?"Yes":"No");

        /* Try execution of data with DEP on*/
        info.exception_caught = FALSE;
        run_exception_test(dpe_exception_handler, &info, single_ret, sizeof(single_ret), PAGE_READWRITE);
        if(has_hw_support)
        {
            ok(info.exception_caught == TRUE, "Execution of data memory succeeded\n");
            ok(info.exception_info == EXCEPTION_EXECUTE_FAULT,
                  "Access violation type: %08x\n", (unsigned)info.exception_info);
        }
        else
            ok(info.exception_caught == FALSE, "Execution trapped without hardware support\n");
    }
    else
        skip("DEP is in AlwaysOff state\n");

    if(!is_permanent)
    {
        /* Disable DEP */
        val = MEM_EXECUTE_OPTION_ENABLE;
        stat = pNtSetInformationProcess(GetCurrentProcess(), ProcessExecuteFlags, &val, sizeof val);
        ok(stat == STATUS_SUCCESS, "disabling DEP: status %08x\n", stat);
    }

    /* page is read without exec here */
    if(can_test_without)
    {
        /* Try execution of data with DEP off */
        info.exception_caught = FALSE;
        run_exception_test(dpe_exception_handler, &info, single_ret, sizeof(single_ret), PAGE_READWRITE);
        ok(info.exception_caught == FALSE, "Execution trapped with DEP turned off\n");

        /* Try access to locked page with DEP off - error code is different than
           with hardware DEP on */
        info.exception_caught = FALSE;
        run_exception_test(dpe_exception_handler, &info, single_ret, sizeof(single_ret), PAGE_NOACCESS);
        ok(info.exception_caught == TRUE, "Execution of disabled memory succeeded\n");
        ok(info.exception_info == EXCEPTION_READ_FAULT,
              "Access violation type: %08x\n", (unsigned)info.exception_info);
    }
    else
        skip("DEP is in AlwaysOn state\n");

    if(!is_permanent)
    {
        /* Turn off DEP permanently */
        val = MEM_EXECUTE_OPTION_ENABLE | MEM_EXECUTE_OPTION_PERMANENT;
        stat = pNtSetInformationProcess(GetCurrentProcess(), ProcessExecuteFlags, &val, sizeof val);
        ok(stat == STATUS_SUCCESS, "disabling DEP permanently: status %08x\n", stat);
    }

    /* Try to turn off DEP */
    val = MEM_EXECUTE_OPTION_ENABLE;
    stat = pNtSetInformationProcess(GetCurrentProcess(), ProcessExecuteFlags, &val, sizeof val);
    ok(stat == STATUS_ACCESS_DENIED, "disabling DEP while permanent: status %08x\n", stat);

    /* Try to turn on DEP */
    val = MEM_EXECUTE_OPTION_DISABLE;
    stat = pNtSetInformationProcess(GetCurrentProcess(), ProcessExecuteFlags, &val, sizeof val);
    ok(stat == STATUS_ACCESS_DENIED, "enabling DEP while permanent: status %08x\n", stat);
}

static void test_thread_context(void)
{
    CONTEXT context;
    NTSTATUS status;
    struct expected
    {
        DWORD Eax, Ebx, Ecx, Edx, Esi, Edi, Ebp, Esp, Eip,
            SegCs, SegDs, SegEs, SegFs, SegGs, SegSs, EFlags, prev_frame,
            x87_control;
    } expect;
    NTSTATUS (*func_ptr)( struct expected *res, void *func, void *arg1, void *arg2,
            DWORD *new_x87_control ) = code_mem;
    DWORD new_x87_control;

    static const BYTE call_func[] =
    {
        0x55,             /* pushl  %ebp */
        0x89, 0xe5,       /* mov    %esp,%ebp */
        0x50,             /* pushl  %eax ; add a bit of offset to the stack */
        0x50,             /* pushl  %eax */
        0x50,             /* pushl  %eax */
        0x50,             /* pushl  %eax */
        0x8b, 0x45, 0x08, /* mov    0x8(%ebp),%eax */
        0x8f, 0x00,       /* popl   (%eax) */
        0x89, 0x58, 0x04, /* mov    %ebx,0x4(%eax) */
        0x89, 0x48, 0x08, /* mov    %ecx,0x8(%eax) */
        0x89, 0x50, 0x0c, /* mov    %edx,0xc(%eax) */
        0x89, 0x70, 0x10, /* mov    %esi,0x10(%eax) */
        0x89, 0x78, 0x14, /* mov    %edi,0x14(%eax) */
        0x89, 0x68, 0x18, /* mov    %ebp,0x18(%eax) */
        0x89, 0x60, 0x1c, /* mov    %esp,0x1c(%eax) */
        0xff, 0x75, 0x04, /* pushl  0x4(%ebp) */
        0x8f, 0x40, 0x20, /* popl   0x20(%eax) */
        0x8c, 0x48, 0x24, /* mov    %cs,0x24(%eax) */
        0x8c, 0x58, 0x28, /* mov    %ds,0x28(%eax) */
        0x8c, 0x40, 0x2c, /* mov    %es,0x2c(%eax) */
        0x8c, 0x60, 0x30, /* mov    %fs,0x30(%eax) */
        0x8c, 0x68, 0x34, /* mov    %gs,0x34(%eax) */
        0x8c, 0x50, 0x38, /* mov    %ss,0x38(%eax) */
        0x9c,             /* pushf */
        0x8f, 0x40, 0x3c, /* popl   0x3c(%eax) */
        0xff, 0x75, 0x00, /* pushl  0x0(%ebp) ; previous stack frame */
        0x8f, 0x40, 0x40, /* popl   0x40(%eax) */
                          /* pushl $0x47f */
        0x68, 0x7f, 0x04, 0x00, 0x00,
        0x8f, 0x40, 0x44, /* popl  0x44(%eax) */
        0xd9, 0x68, 0x44, /* fldcw 0x44(%eax) */

        0x8b, 0x00,       /* mov    (%eax),%eax */
        0xff, 0x75, 0x14, /* pushl  0x14(%ebp) */
        0xff, 0x75, 0x10, /* pushl  0x10(%ebp) */
        0xff, 0x55, 0x0c, /* call   *0xc(%ebp) */

        0x8b, 0x55, 0x18, /* mov   0x18(%ebp),%edx */
        0x9b, 0xd9, 0x3a, /* fstcw (%edx) */
        0xdb, 0xe3,       /* fninit */

        0xc9,             /* leave */
        0xc3,             /* ret */
    };

    memcpy( func_ptr, call_func, sizeof(call_func) );

#define COMPARE(reg) \
    ok( context.reg == expect.reg, "wrong " #reg " %08x/%08x\n", context.reg, expect.reg )

    memset( &context, 0xcc, sizeof(context) );
    memset( &expect, 0xcc, sizeof(expect) );
    func_ptr( &expect, pRtlCaptureContext, &context, 0, &new_x87_control );
    trace( "expect: eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x ebp=%08x esp=%08x "
           "eip=%08x cs=%04x ds=%04x es=%04x fs=%04x gs=%04x ss=%04x flags=%08x prev=%08x\n",
           expect.Eax, expect.Ebx, expect.Ecx, expect.Edx, expect.Esi, expect.Edi,
           expect.Ebp, expect.Esp, expect.Eip, expect.SegCs, expect.SegDs, expect.SegEs,
           expect.SegFs, expect.SegGs, expect.SegSs, expect.EFlags, expect.prev_frame );
    trace( "actual: eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x ebp=%08x esp=%08x "
           "eip=%08x cs=%04x ds=%04x es=%04x fs=%04x gs=%04x ss=%04x flags=%08x\n",
           context.Eax, context.Ebx, context.Ecx, context.Edx, context.Esi, context.Edi,
           context.Ebp, context.Esp, context.Eip, context.SegCs, context.SegDs, context.SegEs,
           context.SegFs, context.SegGs, context.SegSs, context.EFlags );

    ok( context.ContextFlags == (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS) ||
        broken( context.ContextFlags == 0xcccccccc ),  /* <= vista */
        "wrong flags %08x\n", context.ContextFlags );
    COMPARE( Eax );
    COMPARE( Ebx );
    COMPARE( Ecx );
    COMPARE( Edx );
    COMPARE( Esi );
    COMPARE( Edi );
    COMPARE( Eip );
    COMPARE( SegCs );
    COMPARE( SegDs );
    COMPARE( SegEs );
    COMPARE( SegFs );
    COMPARE( SegGs );
    COMPARE( SegSs );
    COMPARE( EFlags );
    /* Ebp is from the previous stackframe */
    ok( context.Ebp == expect.prev_frame, "wrong Ebp %08x/%08x\n", context.Ebp, expect.prev_frame );
    /* Esp is the value on entry to the previous stackframe */
    ok( context.Esp == expect.Ebp + 8, "wrong Esp %08x/%08x\n", context.Esp, expect.Ebp + 8 );

    memset( &context, 0xcc, sizeof(context) );
    memset( &expect, 0xcc, sizeof(expect) );
    context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS | CONTEXT_FLOATING_POINT;

    status = func_ptr( &expect, pNtGetContextThread, (void *)GetCurrentThread(), &context, &new_x87_control );
    ok( status == STATUS_SUCCESS, "NtGetContextThread failed %08x\n", status );
    trace( "expect: eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x ebp=%08x esp=%08x "
           "eip=%08x cs=%04x ds=%04x es=%04x fs=%04x gs=%04x ss=%04x flags=%08x prev=%08x\n",
           expect.Eax, expect.Ebx, expect.Ecx, expect.Edx, expect.Esi, expect.Edi,
           expect.Ebp, expect.Esp, expect.Eip, expect.SegCs, expect.SegDs, expect.SegEs,
           expect.SegFs, expect.SegGs, expect.SegSs, expect.EFlags, expect.prev_frame );
    trace( "actual: eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x ebp=%08x esp=%08x "
           "eip=%08x cs=%04x ds=%04x es=%04x fs=%04x gs=%04x ss=%04x flags=%08x\n",
           context.Eax, context.Ebx, context.Ecx, context.Edx, context.Esi, context.Edi,
           context.Ebp, context.Esp, context.Eip, context.SegCs, context.SegDs, context.SegEs,
           context.SegFs, context.SegGs, context.SegSs, context.EFlags );
    /* Eax, Ecx, Edx, EFlags are not preserved */
    COMPARE( Ebx );
    COMPARE( Esi );
    COMPARE( Edi );
    COMPARE( Ebp );
    /* Esp is the stack upon entry to NtGetContextThread */
    ok( context.Esp == expect.Esp - 12 || context.Esp == expect.Esp - 16,
        "wrong Esp %08x/%08x\n", context.Esp, expect.Esp );
    /* Eip is somewhere close to the NtGetContextThread implementation */
    ok( (char *)context.Eip >= (char *)pNtGetContextThread - 0x40000 &&
        (char *)context.Eip <= (char *)pNtGetContextThread + 0x40000,
        "wrong Eip %08x/%08x\n", context.Eip, (DWORD)pNtGetContextThread );
    /* segment registers clear the high word */
    ok( context.SegCs == LOWORD(expect.SegCs), "wrong SegCs %08x/%08x\n", context.SegCs, expect.SegCs );
    ok( context.SegDs == LOWORD(expect.SegDs), "wrong SegDs %08x/%08x\n", context.SegDs, expect.SegDs );
    ok( context.SegEs == LOWORD(expect.SegEs), "wrong SegEs %08x/%08x\n", context.SegEs, expect.SegEs );
    ok( context.SegFs == LOWORD(expect.SegFs), "wrong SegFs %08x/%08x\n", context.SegFs, expect.SegFs );
    ok( context.SegGs == LOWORD(expect.SegGs), "wrong SegGs %08x/%08x\n", context.SegGs, expect.SegGs );
    ok( context.SegSs == LOWORD(expect.SegSs), "wrong SegSs %08x/%08x\n", context.SegSs, expect.SegGs );

    ok( LOWORD(context.FloatSave.ControlWord) == LOWORD(expect.x87_control),
            "wrong x87 control word %#x/%#x.\n", context.FloatSave.ControlWord, expect.x87_control );
    ok( LOWORD(expect.x87_control) == LOWORD(new_x87_control),
            "x87 control word changed in NtGetContextThread() %#x/%#x.\n",
            LOWORD(expect.x87_control), LOWORD(new_x87_control) );

#undef COMPARE
}

static BYTE saved_KiUserExceptionDispatcher_bytes[7];
static void *pKiUserExceptionDispatcher;
static BOOL hook_called;
static void *hook_KiUserExceptionDispatcher_eip;
static void *dbg_except_continue_handler_eip;
static void *hook_exception_address;

static struct
{
    DWORD old_eax;
    DWORD old_edx;
    DWORD old_esi;
    DWORD old_edi;
    DWORD old_ebp;
    DWORD old_esp;
    DWORD new_eax;
    DWORD new_edx;
    DWORD new_esi;
    DWORD new_edi;
    DWORD new_ebp;
    DWORD new_esp;
}
test_kiuserexceptiondispatcher_regs;

static DWORD dbg_except_continue_handler(EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
        CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher)
{
    ok(hook_called, "Hook was not called.\n");

    ok(rec->ExceptionCode == 0x80000003, "Got unexpected ExceptionCode %#x.\n", rec->ExceptionCode);

    got_exception = 1;
    dbg_except_continue_handler_eip = (void *)context->Eip;
    ++context->Eip;

    context->Eip = (DWORD)code_mem + 0x1c;
    context->Eax = 0xdeadbeef;
    context->Esi = 0xdeadbeef;
    pRtlUnwind(NtCurrentTeb()->Tib.ExceptionList, (void *)context->Eip, rec, (void *)0xdeadbeef);
    return ExceptionContinueExecution;
}

static LONG WINAPI dbg_except_continue_vectored_handler(struct _EXCEPTION_POINTERS *e)
{
    EXCEPTION_RECORD *rec = e->ExceptionRecord;
    CONTEXT *context = e->ContextRecord;

    trace("dbg_except_continue_vectored_handler, code %#x, eip %#x, ExceptionAddress %p.\n",
            rec->ExceptionCode, context->Eip, rec->ExceptionAddress);

    ok(rec->ExceptionCode == 0x80000003, "Got unexpected ExceptionCode %#x.\n", rec->ExceptionCode);

    got_exception = 1;

    if ((ULONG_PTR)rec->ExceptionAddress == context->Eip + 1)
    {
        /* XP and Vista+ have ExceptionAddress == Eip + 1, Eip is adjusted even
         * for software raised breakpoint exception.
         * Win2003 has Eip not adjusted and matching ExceptionAddress.
         * Win2008 has Eip not adjuated and ExceptionAddress not filled for
         * software raised exception. */
        context->Eip = (ULONG_PTR)rec->ExceptionAddress;
    }

    return EXCEPTION_CONTINUE_EXECUTION;
}

/* Use CDECL to leave arguments on stack. */
static void CDECL hook_KiUserExceptionDispatcher(EXCEPTION_RECORD *rec, CONTEXT *context)
{
    trace("rec %p, context %p.\n", rec, context);
    trace("context->Eip %#x, context->Esp %#x, ContextFlags %#x.\n",
            context->Eip, context->Esp, context->ContextFlags);

    hook_called = TRUE;
    /* Broken on Win2008, probably rec offset in stack is different. */
    ok(rec->ExceptionCode == 0x80000003 || broken(!rec->ExceptionCode),
            "Got unexpected ExceptionCode %#x.\n", rec->ExceptionCode);

    hook_KiUserExceptionDispatcher_eip = (void *)context->Eip;
    hook_exception_address = rec->ExceptionAddress;
    memcpy(pKiUserExceptionDispatcher, saved_KiUserExceptionDispatcher_bytes,
            sizeof(saved_KiUserExceptionDispatcher_bytes));
}

static void test_kiuserexceptiondispatcher(void)
{
    PVOID vectored_handler;
    HMODULE hntdll = GetModuleHandleA("ntdll.dll");
    static BYTE except_code[] =
    {
        0xb9, /* mov imm32, %ecx */
        /* offset: 0x1 */
        0x00, 0x00, 0x00, 0x00,

        0x89, 0x01,       /* mov %eax, (%ecx) */
        0x89, 0x51, 0x04, /* mov %edx, 0x4(%ecx) */
        0x89, 0x71, 0x08, /* mov %esi, 0x8(%ecx) */
        0x89, 0x79, 0x0c, /* mov %edi, 0xc(%ecx) */
        0x89, 0x69, 0x10, /* mov %ebp, 0x10(%ecx) */
        0x89, 0x61, 0x14, /* mov %esp, 0x14(%ecx) */
        0x83, 0xc1, 0x18, /* add $0x18, %ecx */

        /* offset: 0x19 */
        0xcc,  /* int3 */

        0x0f, 0x0b, /* ud2, illegal instruction */

        /* offset: 0x1c */
        0xb9, /* mov imm32, %ecx */
        /* offset: 0x1d */
        0x00, 0x00, 0x00, 0x00,

        0x89, 0x01,       /* mov %eax, (%ecx) */
        0x89, 0x51, 0x04, /* mov %edx, 0x4(%ecx) */
        0x89, 0x71, 0x08, /* mov %esi, 0x8(%ecx) */
        0x89, 0x79, 0x0c, /* mov %edi, 0xc(%ecx) */
        0x89, 0x69, 0x10, /* mov %ebp, 0x10(%ecx) */
        0x89, 0x61, 0x14, /* mov %esp, 0x14(%ecx) */

        0x67, 0x48, 0x8b, 0x71, 0xf0, /* mov -0x10(%ecx),%esi */

        0xc3,  /* ret  */
    };
    static BYTE hook_trampoline[] =
    {
        0xff, 0x15,
        /* offset: 2 bytes */
        0x00, 0x00, 0x00, 0x00,     /* callq *addr */ /* call hook implementation. */

        0xff, 0x25,
        /* offset: 8 bytes */
        0x00, 0x00, 0x00, 0x00,     /* jmpq *addr */ /* jump to original function. */
    };
    void *phook_KiUserExceptionDispatcher = hook_KiUserExceptionDispatcher;
    BYTE patched_KiUserExceptionDispatcher_bytes[7];
    void *phook_trampoline = hook_trampoline;
    DWORD old_protect1, old_protect2;
    EXCEPTION_RECORD record;
    void *bpt_address;
    BYTE *ptr;
    BOOL ret;

    pKiUserExceptionDispatcher = (void *)GetProcAddress(hntdll, "KiUserExceptionDispatcher");
    if (!pKiUserExceptionDispatcher)
    {
        win_skip("KiUserExceptionDispatcher is not available.\n");
        return;
    }

    if (!pRtlUnwind)
    {
        win_skip("RtlUnwind is not available.\n");
        return;
    }

    *(DWORD *)(except_code + 1) = (DWORD)&test_kiuserexceptiondispatcher_regs;
    *(DWORD *)(except_code + 0x1d) = (DWORD)&test_kiuserexceptiondispatcher_regs.new_eax;

    *(unsigned int *)(hook_trampoline + 2) = (ULONG_PTR)&phook_KiUserExceptionDispatcher;
    *(unsigned int *)(hook_trampoline + 8) = (ULONG_PTR)&pKiUserExceptionDispatcher;

    ret = VirtualProtect(hook_trampoline, ARRAY_SIZE(hook_trampoline), PAGE_EXECUTE_READWRITE, &old_protect1);
    ok(ret, "Got unexpected ret %#x, GetLastError() %u.\n", ret, GetLastError());

    ret = VirtualProtect(pKiUserExceptionDispatcher, sizeof(saved_KiUserExceptionDispatcher_bytes),
            PAGE_EXECUTE_READWRITE, &old_protect2);
    ok(ret, "Got unexpected ret %#x, GetLastError() %u.\n", ret, GetLastError());

    memcpy(saved_KiUserExceptionDispatcher_bytes, pKiUserExceptionDispatcher,
            sizeof(saved_KiUserExceptionDispatcher_bytes));

    ptr = patched_KiUserExceptionDispatcher_bytes;
    /* mov hook_trampoline, %eax */
    *ptr++ = 0xa1;
    *(void **)ptr = &phook_trampoline;
    ptr += sizeof(void *);
    /* jmp *eax */
    *ptr++ = 0xff;
    *ptr++ = 0xe0;

    memcpy(pKiUserExceptionDispatcher, patched_KiUserExceptionDispatcher_bytes,
            sizeof(patched_KiUserExceptionDispatcher_bytes));
    got_exception = 0;
    run_exception_test(dbg_except_continue_handler, NULL, except_code, ARRAY_SIZE(except_code),
            PAGE_EXECUTE_READ);

    ok(got_exception, "Handler was not called.\n");
    ok(hook_called, "Hook was not called.\n");

    ok(test_kiuserexceptiondispatcher_regs.new_eax == 0xdeadbeef, "Got unexpected eax %#x.\n",
            test_kiuserexceptiondispatcher_regs.new_eax);
    ok(test_kiuserexceptiondispatcher_regs.new_esi == 0xdeadbeef, "Got unexpected esi %#x.\n",
            test_kiuserexceptiondispatcher_regs.new_esi);
    ok(test_kiuserexceptiondispatcher_regs.old_edi
            == test_kiuserexceptiondispatcher_regs.new_edi, "edi does not match.\n");
    ok(test_kiuserexceptiondispatcher_regs.old_ebp
            == test_kiuserexceptiondispatcher_regs.new_ebp, "ebp does not match.\n");

    bpt_address = (BYTE *)code_mem + 0x19;

    ok(hook_exception_address == bpt_address || broken(!hook_exception_address) /* Win2008 */,
            "Got unexpected exception address %p, expected %p.\n",
            hook_exception_address, bpt_address);
    ok(hook_KiUserExceptionDispatcher_eip == bpt_address, "Got unexpected exception address %p, expected %p.\n",
            hook_KiUserExceptionDispatcher_eip, bpt_address);
    ok(dbg_except_continue_handler_eip == bpt_address, "Got unexpected exception address %p, expected %p.\n",
            dbg_except_continue_handler_eip, bpt_address);

    record.ExceptionCode = 0x80000003;
    record.ExceptionFlags = 0;
    record.ExceptionRecord = NULL;
    record.ExceptionAddress = NULL; /* does not matter, copied return address */
    record.NumberParameters = 0;

    vectored_handler = AddVectoredExceptionHandler(TRUE, dbg_except_continue_vectored_handler);

    memcpy(pKiUserExceptionDispatcher, patched_KiUserExceptionDispatcher_bytes,
            sizeof(patched_KiUserExceptionDispatcher_bytes));
    got_exception = 0;
    hook_called = FALSE;

    pRtlRaiseException(&record);

    ok(got_exception, "Handler was not called.\n");
    ok(hook_called || broken(!hook_called) /* 2003 */, "Hook was not called.\n");

    memcpy(pKiUserExceptionDispatcher, saved_KiUserExceptionDispatcher_bytes,
            sizeof(saved_KiUserExceptionDispatcher_bytes));

    RemoveVectoredExceptionHandler(vectored_handler);
    ret = VirtualProtect(pKiUserExceptionDispatcher, sizeof(saved_KiUserExceptionDispatcher_bytes),
            old_protect2, &old_protect2);
    ok(ret, "Got unexpected ret %#x, GetLastError() %u.\n", ret, GetLastError());
    ret = VirtualProtect(hook_trampoline, ARRAY_SIZE(hook_trampoline), old_protect1, &old_protect1);
    ok(ret, "Got unexpected ret %#x, GetLastError() %u.\n", ret, GetLastError());
}

#elif defined(__x86_64__)

#define UNW_FLAG_NHANDLER  0
#define UNW_FLAG_EHANDLER  1
#define UNW_FLAG_UHANDLER  2
#define UNW_FLAG_CHAININFO 4

#define UWOP_PUSH_NONVOL     0
#define UWOP_ALLOC_LARGE     1
#define UWOP_ALLOC_SMALL     2
#define UWOP_SET_FPREG       3
#define UWOP_SAVE_NONVOL     4
#define UWOP_SAVE_NONVOL_FAR 5
#define UWOP_SAVE_XMM128     8
#define UWOP_SAVE_XMM128_FAR 9
#define UWOP_PUSH_MACHFRAME  10

struct results
{
    int rip_offset;   /* rip offset from code start */
    int rbp_offset;   /* rbp offset from stack pointer */
    int handler;      /* expect handler to be set? */
    int rip;          /* expected final rip value */
    int frame;        /* expected frame return value */
    int regs[8][2];   /* expected values for registers */
};

struct unwind_test
{
    const BYTE *function;
    size_t function_size;
    const BYTE *unwind_info;
    const struct results *results;
    unsigned int nb_results;
    const struct results *broken_results;
};

enum regs
{
    rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi,
    r8,  r9,  r10, r11, r12, r13, r14, r15
};

static const char * const reg_names[16] =
{
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"
};

#define UWOP(code,info) (UWOP_##code | ((info) << 4))

static void call_virtual_unwind( int testnum, const struct unwind_test *test )
{
    static const int code_offset = 1024;
    static const int unwind_offset = 2048;
    void *handler, *data;
    CONTEXT context;
    RUNTIME_FUNCTION runtime_func;
    KNONVOLATILE_CONTEXT_POINTERS ctx_ptr;
    UINT i, j, k, broken_k;
    ULONG64 fake_stack[256];
    ULONG64 frame, orig_rip, orig_rbp, unset_reg;
    UINT unwind_size = 4 + 2 * test->unwind_info[2] + 8;
    void *expected_handler, *broken_handler;

    memcpy( (char *)code_mem + code_offset, test->function, test->function_size );
    memcpy( (char *)code_mem + unwind_offset, test->unwind_info, unwind_size );

    runtime_func.BeginAddress = code_offset;
    runtime_func.EndAddress = code_offset + test->function_size;
    runtime_func.UnwindData = unwind_offset;

    trace( "code: %p stack: %p\n", code_mem, fake_stack );

    for (i = 0; i < test->nb_results; i++)
    {
        memset( &ctx_ptr, 0, sizeof(ctx_ptr) );
        memset( &context, 0x55, sizeof(context) );
        memset( &unset_reg, 0x55, sizeof(unset_reg) );
        for (j = 0; j < 256; j++) fake_stack[j] = j * 8;

        context.Rsp = (ULONG_PTR)fake_stack;
        context.Rbp = (ULONG_PTR)fake_stack + test->results[i].rbp_offset;
        orig_rbp = context.Rbp;
        orig_rip = (ULONG64)code_mem + code_offset + test->results[i].rip_offset;

        trace( "%u/%u: rip=%p (%02x) rbp=%p rsp=%p\n", testnum, i,
               (void *)orig_rip, *(BYTE *)orig_rip, (void *)orig_rbp, (void *)context.Rsp );

        data = (void *)0xdeadbeef;
        handler = RtlVirtualUnwind( UNW_FLAG_EHANDLER, (ULONG64)code_mem, orig_rip,
                                    &runtime_func, &context, &data, &frame, &ctx_ptr );

        expected_handler = test->results[i].handler ? (char *)code_mem + 0x200 : NULL;
        broken_handler = test->broken_results && test->broken_results[i].handler ? (char *)code_mem + 0x200 : NULL;

        ok( handler == expected_handler || broken( test->broken_results && handler == broken_handler ),
                "%u/%u: wrong handler %p/%p\n", testnum, i, handler, expected_handler );
        if (handler)
            ok( *(DWORD *)data == 0x08070605, "%u/%u: wrong handler data %p\n", testnum, i, data );
        else
            ok( data == (void *)0xdeadbeef, "%u/%u: handler data set to %p\n", testnum, i, data );

        ok( context.Rip == test->results[i].rip
                || broken( test->broken_results && context.Rip == test->broken_results[i].rip ),
                "%u/%u: wrong rip %p/%x\n", testnum, i, (void *)context.Rip, test->results[i].rip );
        ok( frame == (ULONG64)fake_stack + test->results[i].frame
                || broken( test->broken_results && frame == (ULONG64)fake_stack + test->broken_results[i].frame ),
                "%u/%u: wrong frame %p/%p\n",
                testnum, i, (void *)frame, (char *)fake_stack + test->results[i].frame );

        for (j = 0; j < 16; j++)
        {
            static const UINT nb_regs = ARRAY_SIZE(test->results[i].regs);

            for (k = 0; k < nb_regs; k++)
            {
                if (test->results[i].regs[k][0] == -1)
                {
                    k = nb_regs;
                    break;
                }
                if (test->results[i].regs[k][0] == j) break;
            }

            if (test->broken_results)
            {
                for (broken_k = 0; broken_k < nb_regs; broken_k++)
                {
                    if (test->broken_results[i].regs[broken_k][0] == -1)
                    {
                        broken_k = nb_regs;
                        break;
                    }
                    if (test->broken_results[i].regs[broken_k][0] == j)
                        break;
                }
            }
            else
            {
                broken_k = k;
            }

            if (j == rsp)  /* rsp is special */
            {
                ULONG64 expected_rsp, broken_rsp;

                ok( !ctx_ptr.IntegerContext[j],
                    "%u/%u: rsp should not be set in ctx_ptr\n", testnum, i );
                expected_rsp = test->results[i].regs[k][1] < 0
                        ? -test->results[i].regs[k][1] : (ULONG64)fake_stack + test->results[i].regs[k][1];
                if (test->broken_results)
                    broken_rsp = test->broken_results[i].regs[k][1] < 0
                            ? -test->broken_results[i].regs[k][1]
                            : (ULONG64)fake_stack + test->broken_results[i].regs[k][1];
                else
                    broken_rsp = expected_rsp;

                ok( context.Rsp == expected_rsp || broken( context.Rsp == broken_rsp ),
                    "%u/%u: register rsp wrong %p/%p\n",
                    testnum, i, (void *)context.Rsp, (void *)expected_rsp );
                continue;
            }

            if (ctx_ptr.IntegerContext[j])
            {
                ok( k < nb_regs || broken( broken_k < nb_regs ), "%u/%u: register %s should not be set to %lx\n",
                    testnum, i, reg_names[j], *(&context.Rax + j) );
                ok( k == nb_regs || *(&context.Rax + j) == test->results[i].regs[k][1]
                        || broken( broken_k == nb_regs || *(&context.Rax + j)
                        == test->broken_results[i].regs[broken_k][1] ),
                        "%u/%u: register %s wrong %p/%x\n",
                        testnum, i, reg_names[j], (void *)*(&context.Rax + j), test->results[i].regs[k][1] );
            }
            else
            {
                ok( k == nb_regs || broken( broken_k == nb_regs ), "%u/%u: register %s should be set\n",
                        testnum, i, reg_names[j] );
                if (j == rbp)
                    ok( context.Rbp == orig_rbp, "%u/%u: register rbp wrong %p/unset\n",
                        testnum, i, (void *)context.Rbp );
                else
                    ok( *(&context.Rax + j) == unset_reg,
                        "%u/%u: register %s wrong %p/unset\n",
                        testnum, i, reg_names[j], (void *)*(&context.Rax + j));
            }
        }
    }
}

static void test_virtual_unwind(void)
{
    static const BYTE function_0[] =
    {
        0xff, 0xf5,                                  /* 00: push %rbp */
        0x48, 0x81, 0xec, 0x10, 0x01, 0x00, 0x00,    /* 02: sub $0x110,%rsp */
        0x48, 0x8d, 0x6c, 0x24, 0x30,                /* 09: lea 0x30(%rsp),%rbp */
        0x48, 0x89, 0x9d, 0xf0, 0x00, 0x00, 0x00,    /* 0e: mov %rbx,0xf0(%rbp) */
        0x48, 0x89, 0xb5, 0xf8, 0x00, 0x00, 0x00,    /* 15: mov %rsi,0xf8(%rbp) */
        0x90,                                        /* 1c: nop */
        0x48, 0x8b, 0x9d, 0xf0, 0x00, 0x00, 0x00,    /* 1d: mov 0xf0(%rbp),%rbx */
        0x48, 0x8b, 0xb5, 0xf8, 0x00, 0x00, 0x00,    /* 24: mov 0xf8(%rbp),%rsi */
        0x48, 0x8d, 0xa5, 0xe0, 0x00, 0x00, 0x00,    /* 2b: lea 0xe0(%rbp),%rsp */
        0x5d,                                        /* 32: pop %rbp */
        0xc3                                         /* 33: ret */
    };

    static const BYTE unwind_info_0[] =
    {
        1 | (UNW_FLAG_EHANDLER << 3),  /* version + flags */
        0x1c,                          /* prolog size */
        8,                             /* opcode count */
        (0x03 << 4) | rbp,             /* frame reg rbp offset 0x30 */

        0x1c, UWOP(SAVE_NONVOL, rsi), 0x25, 0, /* 1c: mov %rsi,0x128(%rsp) */
        0x15, UWOP(SAVE_NONVOL, rbx), 0x24, 0, /* 15: mov %rbx,0x120(%rsp) */
        0x0e, UWOP(SET_FPREG, rbp),            /* 0e: lea 0x30(%rsp),rbp */
        0x09, UWOP(ALLOC_LARGE, 0), 0x22, 0,   /* 09: sub $0x110,%rsp */
        0x02, UWOP(PUSH_NONVOL, rbp),          /* 02: push %rbp */

        0x00, 0x02, 0x00, 0x00,  /* handler */
        0x05, 0x06, 0x07, 0x08,  /* data */
    };

    static const struct results results_0[] =
    {
      /* offset  rbp   handler  rip   frame   registers */
        { 0x00,  0x40,  FALSE, 0x000, 0x000, { {rsp,0x008}, {-1,-1} }},
        { 0x02,  0x40,  FALSE, 0x008, 0x000, { {rsp,0x010}, {rbp,0x000}, {-1,-1} }},
        { 0x09,  0x40,  FALSE, 0x118, 0x000, { {rsp,0x120}, {rbp,0x110}, {-1,-1} }},
        { 0x0e,  0x40,  FALSE, 0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {-1,-1} }},
        { 0x15,  0x40,  FALSE, 0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {rbx,0x130}, {-1,-1} }},
        { 0x1c,  0x40,  TRUE,  0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {rbx,0x130}, {rsi,0x138}, {-1,-1}}},
        { 0x1d,  0x40,  TRUE,  0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {rbx,0x130}, {rsi,0x138}, {-1,-1}}},
        { 0x24,  0x40,  TRUE,  0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {rbx,0x130}, {rsi,0x138}, {-1,-1}}},
        { 0x2b,  0x40,  FALSE, 0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {-1,-1}}},
        { 0x32,  0x40,  FALSE, 0x008, 0x010, { {rsp,0x010}, {rbp,0x000}, {-1,-1}}},
        { 0x33,  0x40,  FALSE, 0x000, 0x010, { {rsp,0x008}, {-1,-1}}},
    };


    static const BYTE function_1[] =
    {
        0x53,                     /* 00: push %rbx */
        0x55,                     /* 01: push %rbp */
        0x56,                     /* 02: push %rsi */
        0x57,                     /* 03: push %rdi */
        0x41, 0x54,               /* 04: push %r12 */
        0x48, 0x83, 0xec, 0x30,   /* 06: sub $0x30,%rsp */
        0x90, 0x90,               /* 0a: nop; nop */
        0x48, 0x83, 0xc4, 0x30,   /* 0c: add $0x30,%rsp */
        0x41, 0x5c,               /* 10: pop %r12 */
        0x5f,                     /* 12: pop %rdi */
        0x5e,                     /* 13: pop %rsi */
        0x5d,                     /* 14: pop %rbp */
        0x5b,                     /* 15: pop %rbx */
        0xc3                      /* 16: ret */
     };

    static const BYTE unwind_info_1[] =
    {
        1 | (UNW_FLAG_EHANDLER << 3),  /* version + flags */
        0x0a,                          /* prolog size */
        6,                             /* opcode count */
        0,                             /* frame reg */

        0x0a, UWOP(ALLOC_SMALL, 5),   /* 0a: sub $0x30,%rsp */
        0x06, UWOP(PUSH_NONVOL, r12), /* 06: push %r12 */
        0x04, UWOP(PUSH_NONVOL, rdi), /* 04: push %rdi */
        0x03, UWOP(PUSH_NONVOL, rsi), /* 03: push %rsi */
        0x02, UWOP(PUSH_NONVOL, rbp), /* 02: push %rbp */
        0x01, UWOP(PUSH_NONVOL, rbx), /* 01: push %rbx */

        0x00, 0x02, 0x00, 0x00,  /* handler */
        0x05, 0x06, 0x07, 0x08,  /* data */
    };

    static const struct results results_1[] =
    {
      /* offset  rbp   handler  rip   frame   registers */
        { 0x00,  0x50,  FALSE, 0x000, 0x000, { {rsp,0x008}, {-1,-1} }},
        { 0x01,  0x50,  FALSE, 0x008, 0x000, { {rsp,0x010}, {rbx,0x000}, {-1,-1} }},
        { 0x02,  0x50,  FALSE, 0x010, 0x000, { {rsp,0x018}, {rbx,0x008}, {rbp,0x000}, {-1,-1} }},
        { 0x03,  0x50,  FALSE, 0x018, 0x000, { {rsp,0x020}, {rbx,0x010}, {rbp,0x008}, {rsi,0x000}, {-1,-1} }},
        { 0x04,  0x50,  FALSE, 0x020, 0x000, { {rsp,0x028}, {rbx,0x018}, {rbp,0x010}, {rsi,0x008}, {rdi,0x000}, {-1,-1} }},
        { 0x06,  0x50,  FALSE, 0x028, 0x000, { {rsp,0x030}, {rbx,0x020}, {rbp,0x018}, {rsi,0x010}, {rdi,0x008}, {r12,0x000}, {-1,-1} }},
        { 0x0a,  0x50,  TRUE,  0x058, 0x000, { {rsp,0x060}, {rbx,0x050}, {rbp,0x048}, {rsi,0x040}, {rdi,0x038}, {r12,0x030}, {-1,-1} }},
        { 0x0c,  0x50,  FALSE, 0x058, 0x000, { {rsp,0x060}, {rbx,0x050}, {rbp,0x048}, {rsi,0x040}, {rdi,0x038}, {r12,0x030}, {-1,-1} }},
        { 0x10,  0x50,  FALSE, 0x028, 0x000, { {rsp,0x030}, {rbx,0x020}, {rbp,0x018}, {rsi,0x010}, {rdi,0x008}, {r12,0x000}, {-1,-1} }},
        { 0x12,  0x50,  FALSE, 0x020, 0x000, { {rsp,0x028}, {rbx,0x018}, {rbp,0x010}, {rsi,0x008}, {rdi,0x000}, {-1,-1} }},
        { 0x13,  0x50,  FALSE, 0x018, 0x000, { {rsp,0x020}, {rbx,0x010}, {rbp,0x008}, {rsi,0x000}, {-1,-1} }},
        { 0x14,  0x50,  FALSE, 0x010, 0x000, { {rsp,0x018}, {rbx,0x008}, {rbp,0x000}, {-1,-1} }},
        { 0x15,  0x50,  FALSE, 0x008, 0x000, { {rsp,0x010}, {rbx,0x000}, {-1,-1} }},
        { 0x16,  0x50,  FALSE, 0x000, 0x000, { {rsp,0x008}, {-1,-1} }},
    };

    static const BYTE function_2[] =
    {
        0x55,                     /* 00: push %rbp */
        0x90, 0x90,               /* 01: nop; nop */
        0x5d,                     /* 03: pop %rbp */
        0xc3                      /* 04: ret */
     };

    static const BYTE unwind_info_2[] =
    {
        1 | (UNW_FLAG_EHANDLER << 3),  /* version + flags */
        0x0,                           /* prolog size */
        2,                             /* opcode count */
        0,                             /* frame reg */

        0x01, UWOP(PUSH_NONVOL, rbp), /* 02: push %rbp */
        0x00, UWOP(PUSH_MACHFRAME, 0), /* 00 */

        0x00, 0x02, 0x00, 0x00,  /* handler */
        0x05, 0x06, 0x07, 0x08,  /* data */
    };

    static const struct results results_2[] =
    {
      /* offset  rbp   handler  rip   frame   registers */
        { 0x01,  0x50,  TRUE, 0x008, 0x000, { {rsp,-0x020}, {rbp,0x000}, {-1,-1} }},
    };

    static const BYTE unwind_info_3[] =
    {
        1 | (UNW_FLAG_EHANDLER << 3),  /* version + flags */
        0x0,                           /* prolog size */
        2,                             /* opcode count */
        0,                             /* frame reg */

        0x01, UWOP(PUSH_NONVOL, rbp), /* 02: push %rbp */
        0x00, UWOP(PUSH_MACHFRAME, 1), /* 00 */

        0x00, 0x02, 0x00, 0x00,  /* handler */
        0x05, 0x06, 0x07, 0x08,  /* data */
    };

    static const struct results results_3[] =
    {
      /* offset  rbp   handler  rip   frame   registers */
        { 0x01,  0x50,  TRUE, 0x010, 0x000, { {rsp,-0x028}, {rbp,0x000}, {-1,-1} }},
    };

    static const BYTE function_4[] =
    {
        0x55,                     /* 00: push %rbp */
        0x5d,                     /* 01: pop %rbp */
        0xc3                      /* 02: ret */
     };

    static const BYTE unwind_info_4[] =
    {
        1 | (UNW_FLAG_EHANDLER << 3),  /* version + flags */
        0x0,                           /* prolog size */
        0,                             /* opcode count */
        0,                             /* frame reg */

        0x00, 0x02, 0x00, 0x00,  /* handler */
        0x05, 0x06, 0x07, 0x08,  /* data */
    };

    static const struct results results_4[] =
    {
      /* offset  rbp   handler  rip   frame   registers */
        { 0x01,  0x50,  TRUE, 0x000, 0x000, { {rsp,0x008}, {-1,-1} }},
    };

    static const struct results broken_results_4[] =
    {
      /* offset  rbp   handler  rip   frame   registers */
        { 0x01,  0x50,  FALSE, 0x008, 0x000, { {rsp,0x010}, {rbp,0x000}, {-1,-1} }},
    };

    static const struct unwind_test tests[] =
    {
        { function_0, sizeof(function_0), unwind_info_0, results_0, ARRAY_SIZE(results_0) },
        { function_1, sizeof(function_1), unwind_info_1, results_1, ARRAY_SIZE(results_1) },
        { function_2, sizeof(function_2), unwind_info_2, results_2, ARRAY_SIZE(results_2) },
        { function_2, sizeof(function_2), unwind_info_3, results_3, ARRAY_SIZE(results_3) },

        /* Broken before Win10 1809. */
        { function_4, sizeof(function_4), unwind_info_4, results_4, ARRAY_SIZE(results_4), broken_results_4 },
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
        call_virtual_unwind( i, &tests[i] );
}

static int consolidate_dummy_called;
static PVOID CALLBACK test_consolidate_dummy(EXCEPTION_RECORD *rec)
{
    CONTEXT *ctx = (CONTEXT *)rec->ExceptionInformation[1];
    consolidate_dummy_called = 1;
    ok(ctx->Rip == 0xdeadbeef, "test_consolidate_dummy failed for Rip, expected: 0xdeadbeef, got: %lx\n", ctx->Rip);
    return (PVOID)rec->ExceptionInformation[2];
}

static void test_restore_context(void)
{
    SETJMP_FLOAT128 *fltsave;
    EXCEPTION_RECORD rec;
    _JUMP_BUFFER buf;
    CONTEXT ctx;
    int i, pass;

    if (!pRtlUnwindEx || !pRtlRestoreContext || !pRtlCaptureContext || !p_setjmp)
    {
        skip("RtlUnwindEx/RtlCaptureContext/RtlRestoreContext/_setjmp not found\n");
        return;
    }

    /* RtlRestoreContext(NULL, NULL); crashes on Windows */

    /* test simple case of capture and restore context */
    pass = 0;
    InterlockedIncrement(&pass); /* interlocked to prevent compiler from moving after capture */
    pRtlCaptureContext(&ctx);
    if (InterlockedIncrement(&pass) == 2) /* interlocked to prevent compiler from moving before capture */
    {
        pRtlRestoreContext(&ctx, NULL);
        ok(0, "shouldn't be reached\n");
    }
    else
        ok(pass < 4, "unexpected pass %d\n", pass);

    /* test with jmp using RltRestoreContext */
    pass = 0;
    InterlockedIncrement(&pass);
    RtlCaptureContext(&ctx);
    InterlockedIncrement(&pass); /* only called once */
    p_setjmp(&buf);
    InterlockedIncrement(&pass);
    if (pass == 3)
    {
        rec.ExceptionCode = STATUS_LONGJUMP;
        rec.NumberParameters = 1;
        rec.ExceptionInformation[0] = (DWORD64)&buf;

        /* uses buf.Rip instead of ctx.Rip */
        pRtlRestoreContext(&ctx, &rec);
        ok(0, "shouldn't be reached\n");
    }
    else if (pass == 4)
    {
        ok(buf.Rbx == ctx.Rbx, "longjmp failed for Rbx, expected: %lx, got: %lx\n", buf.Rbx, ctx.Rbx);
        ok(buf.Rsp == ctx.Rsp, "longjmp failed for Rsp, expected: %lx, got: %lx\n", buf.Rsp, ctx.Rsp);
        ok(buf.Rbp == ctx.Rbp, "longjmp failed for Rbp, expected: %lx, got: %lx\n", buf.Rbp, ctx.Rbp);
        ok(buf.Rsi == ctx.Rsi, "longjmp failed for Rsi, expected: %lx, got: %lx\n", buf.Rsi, ctx.Rsi);
        ok(buf.Rdi == ctx.Rdi, "longjmp failed for Rdi, expected: %lx, got: %lx\n", buf.Rdi, ctx.Rdi);
        ok(buf.R12 == ctx.R12, "longjmp failed for R12, expected: %lx, got: %lx\n", buf.R12, ctx.R12);
        ok(buf.R13 == ctx.R13, "longjmp failed for R13, expected: %lx, got: %lx\n", buf.R13, ctx.R13);
        ok(buf.R14 == ctx.R14, "longjmp failed for R14, expected: %lx, got: %lx\n", buf.R14, ctx.R14);
        ok(buf.R15 == ctx.R15, "longjmp failed for R15, expected: %lx, got: %lx\n", buf.R15, ctx.R15);

        fltsave = &buf.Xmm6;
        for (i = 0; i < 10; i++)
        {
            ok(fltsave[i].Part[0] == ctx.FltSave.XmmRegisters[i + 6].Low,
                "longjmp failed for Xmm%d, expected %lx, got %lx\n", i + 6,
                fltsave[i].Part[0], ctx.FltSave.XmmRegisters[i + 6].Low);

            ok(fltsave[i].Part[1] == ctx.FltSave.XmmRegisters[i + 6].High,
                "longjmp failed for Xmm%d, expected %lx, got %lx\n", i + 6,
                fltsave[i].Part[1], ctx.FltSave.XmmRegisters[i + 6].High);
        }
    }
    else
        ok(0, "unexpected pass %d\n", pass);

    /* test with jmp through RtlUnwindEx */
    pass = 0;
    InterlockedIncrement(&pass);
    pRtlCaptureContext(&ctx);
    InterlockedIncrement(&pass); /* only called once */
    p_setjmp(&buf);
    InterlockedIncrement(&pass);
    if (pass == 3)
    {
        rec.ExceptionCode = STATUS_LONGJUMP;
        rec.NumberParameters = 1;
        rec.ExceptionInformation[0] = (DWORD64)&buf;

        /* uses buf.Rip instead of bogus 0xdeadbeef */
        pRtlUnwindEx((void*)buf.Rsp, (void*)0xdeadbeef, &rec, NULL, &ctx, NULL);
        ok(0, "shouldn't be reached\n");
    }
    else
        ok(pass == 4, "unexpected pass %d\n", pass);


    /* test with consolidate */
    pass = 0;
    InterlockedIncrement(&pass);
    RtlCaptureContext(&ctx);
    InterlockedIncrement(&pass);
    if (pass == 2)
    {
        rec.ExceptionCode = STATUS_UNWIND_CONSOLIDATE;
        rec.NumberParameters = 3;
        rec.ExceptionInformation[0] = (DWORD64)test_consolidate_dummy;
        rec.ExceptionInformation[1] = (DWORD64)&ctx;
        rec.ExceptionInformation[2] = ctx.Rip;
        ctx.Rip = 0xdeadbeef;

        pRtlRestoreContext(&ctx, &rec);
        ok(0, "shouldn't be reached\n");
    }
    else if (pass == 3)
        ok(consolidate_dummy_called, "test_consolidate_dummy not called\n");
    else
        ok(0, "unexpected pass %d\n", pass);
}

static RUNTIME_FUNCTION* CALLBACK dynamic_unwind_callback( DWORD64 pc, PVOID context )
{
    static const int code_offset = 1024;
    static RUNTIME_FUNCTION runtime_func;
    (*(DWORD *)context)++;

    runtime_func.BeginAddress = code_offset + 16;
    runtime_func.EndAddress   = code_offset + 32;
    runtime_func.UnwindData   = 0;
    return &runtime_func;
}

static void test_dynamic_unwind(void)
{
    static const int code_offset = 1024;
    char buf[2 * sizeof(RUNTIME_FUNCTION) + 4];
    RUNTIME_FUNCTION *runtime_func, *func;
    ULONG_PTR table, base;
    void *growable_table;
    NTSTATUS status;
    DWORD count;

    /* Test RtlAddFunctionTable with aligned RUNTIME_FUNCTION pointer */
    runtime_func = (RUNTIME_FUNCTION *)buf;
    runtime_func->BeginAddress = code_offset;
    runtime_func->EndAddress   = code_offset + 16;
    runtime_func->UnwindData   = 0;
    ok( pRtlAddFunctionTable( runtime_func, 1, (ULONG_PTR)code_mem ),
        "RtlAddFunctionTable failed for runtime_func = %p (aligned)\n", runtime_func );

    /* Lookup function outside of any function table */
    base = 0xdeadbeef;
    func = pRtlLookupFunctionEntry( (ULONG_PTR)code_mem + code_offset + 16, &base, NULL );
    ok( func == NULL,
        "RtlLookupFunctionEntry returned unexpected function, expected: NULL, got: %p\n", func );
    ok( !base || broken(base == 0xdeadbeef),
        "RtlLookupFunctionEntry modified base address, expected: 0, got: %lx\n", base );

    /* Test with pointer inside of our function */
    base = 0xdeadbeef;
    func = pRtlLookupFunctionEntry( (ULONG_PTR)code_mem + code_offset + 8, &base, NULL );
    ok( func == runtime_func,
        "RtlLookupFunctionEntry didn't return expected function, expected: %p, got: %p\n", runtime_func, func );
    ok( base == (ULONG_PTR)code_mem,
        "RtlLookupFunctionEntry returned invalid base, expected: %lx, got: %lx\n", (ULONG_PTR)code_mem, base );

    /* Test RtlDeleteFunctionTable */
    ok( pRtlDeleteFunctionTable( runtime_func ),
        "RtlDeleteFunctionTable failed for runtime_func = %p (aligned)\n", runtime_func );
    ok( !pRtlDeleteFunctionTable( runtime_func ),
        "RtlDeleteFunctionTable returned success for nonexistent table runtime_func = %p\n", runtime_func );

    /* Unaligned RUNTIME_FUNCTION pointer */
    runtime_func = (RUNTIME_FUNCTION *)((ULONG_PTR)buf | 0x3);
    runtime_func->BeginAddress = code_offset;
    runtime_func->EndAddress   = code_offset + 16;
    runtime_func->UnwindData   = 0;
    ok( pRtlAddFunctionTable( runtime_func, 1, (ULONG_PTR)code_mem ),
        "RtlAddFunctionTable failed for runtime_func = %p (unaligned)\n", runtime_func );
    ok( pRtlDeleteFunctionTable( runtime_func ),
        "RtlDeleteFunctionTable failed for runtime_func = %p (unaligned)\n", runtime_func );

    /* Attempt to insert the same entry twice */
    runtime_func = (RUNTIME_FUNCTION *)buf;
    runtime_func->BeginAddress = code_offset;
    runtime_func->EndAddress   = code_offset + 16;
    runtime_func->UnwindData   = 0;
    ok( pRtlAddFunctionTable( runtime_func, 1, (ULONG_PTR)code_mem ),
        "RtlAddFunctionTable failed for runtime_func = %p (first attempt)\n", runtime_func );
    ok( pRtlAddFunctionTable( runtime_func, 1, (ULONG_PTR)code_mem ),
        "RtlAddFunctionTable failed for runtime_func = %p (second attempt)\n", runtime_func );
    ok( pRtlDeleteFunctionTable( runtime_func ),
        "RtlDeleteFunctionTable failed for runtime_func = %p (first attempt)\n", runtime_func );
    ok( pRtlDeleteFunctionTable( runtime_func ),
        "RtlDeleteFunctionTable failed for runtime_func = %p (second attempt)\n", runtime_func );
    ok( !pRtlDeleteFunctionTable( runtime_func ),
        "RtlDeleteFunctionTable returned success for nonexistent table runtime_func = %p\n", runtime_func );

    /* Empty table */
    ok( pRtlAddFunctionTable( runtime_func, 0, (ULONG_PTR)code_mem ),
        "RtlAddFunctionTable failed for empty table\n" );
    ok( pRtlDeleteFunctionTable( runtime_func ),
        "RtlDeleteFunctionTable failed for empty table\n" );
    ok( !pRtlDeleteFunctionTable( runtime_func ),
        "RtlDeleteFunctionTable succeeded twice for empty table\n" );

    /* Test RtlInstallFunctionTableCallback with both low bits unset */
    table = (ULONG_PTR)code_mem;
    ok( !pRtlInstallFunctionTableCallback( table, (ULONG_PTR)code_mem, code_offset + 32, &dynamic_unwind_callback, (PVOID*)&count, NULL ),
        "RtlInstallFunctionTableCallback returned success for table = %lx\n", table );

    /* Test RtlInstallFunctionTableCallback with both low bits set */
    table = (ULONG_PTR)code_mem | 0x3;
    ok( pRtlInstallFunctionTableCallback( table, (ULONG_PTR)code_mem, code_offset + 32, &dynamic_unwind_callback, (PVOID*)&count, NULL ),
        "RtlInstallFunctionTableCallback failed for table = %lx\n", table );

    /* Lookup function outside of any function table */
    count = 0;
    base = 0xdeadbeef;
    func = pRtlLookupFunctionEntry( (ULONG_PTR)code_mem + code_offset + 32, &base, NULL );
    ok( func == NULL,
        "RtlLookupFunctionEntry returned unexpected function, expected: NULL, got: %p\n", func );
    ok( !base || broken(base == 0xdeadbeef),
        "RtlLookupFunctionEntry modified base address, expected: 0, got: %lx\n", base );
    ok( !count,
        "RtlLookupFunctionEntry issued %d unexpected calls to dynamic_unwind_callback\n", count );

    /* Test with pointer inside of our function */
    count = 0;
    base = 0xdeadbeef;
    func = pRtlLookupFunctionEntry( (ULONG_PTR)code_mem + code_offset + 24, &base, NULL );
    ok( func != NULL && func->BeginAddress == code_offset + 16 && func->EndAddress == code_offset + 32,
        "RtlLookupFunctionEntry didn't return expected function, got: %p\n", func );
    ok( base == (ULONG_PTR)code_mem,
        "RtlLookupFunctionEntry returned invalid base, expected: %lx, got: %lx\n", (ULONG_PTR)code_mem, base );
    ok( count == 1,
        "RtlLookupFunctionEntry issued %d calls to dynamic_unwind_callback, expected: 1\n", count );

    /* Clean up again */
    ok( pRtlDeleteFunctionTable( (PRUNTIME_FUNCTION)table ),
        "RtlDeleteFunctionTable failed for table = %p\n", (PVOID)table );
    ok( !pRtlDeleteFunctionTable( (PRUNTIME_FUNCTION)table ),
        "RtlDeleteFunctionTable returned success for nonexistent table = %p\n", (PVOID)table );

    if (!pRtlAddGrowableFunctionTable)
    {
        win_skip("Growable function tables are not supported.\n");
        return;
    }

    runtime_func = (RUNTIME_FUNCTION *)buf;
    runtime_func->BeginAddress = code_offset;
    runtime_func->EndAddress   = code_offset + 16;
    runtime_func->UnwindData   = 0;
    runtime_func++;
    runtime_func->BeginAddress = code_offset + 16;
    runtime_func->EndAddress   = code_offset + 32;
    runtime_func->UnwindData   = 0;
    runtime_func = (RUNTIME_FUNCTION *)buf;

    growable_table = NULL;
    status = pRtlAddGrowableFunctionTable( &growable_table, runtime_func, 1, 1, (ULONG_PTR)code_mem, (ULONG_PTR)code_mem + 64 );
    ok(!status, "RtlAddGrowableFunctionTable failed for runtime_func = %p (aligned), %#x.\n", runtime_func, status );
    ok(growable_table != 0, "Unexpected table value.\n");
    pRtlDeleteGrowableFunctionTable( growable_table );

    growable_table = NULL;
    status = pRtlAddGrowableFunctionTable( &growable_table, runtime_func, 2, 2, (ULONG_PTR)code_mem, (ULONG_PTR)code_mem + 64 );
    ok(!status, "RtlAddGrowableFunctionTable failed for runtime_func = %p (aligned), %#x.\n", runtime_func, status );
    ok(growable_table != 0, "Unexpected table value.\n");
    pRtlDeleteGrowableFunctionTable( growable_table );

    growable_table = NULL;
    status = pRtlAddGrowableFunctionTable( &growable_table, runtime_func, 1, 2, (ULONG_PTR)code_mem, (ULONG_PTR)code_mem + 64 );
    ok(!status, "RtlAddGrowableFunctionTable failed for runtime_func = %p (aligned), %#x.\n", runtime_func, status );
    ok(growable_table != 0, "Unexpected table value.\n");
    pRtlDeleteGrowableFunctionTable( growable_table );

    growable_table = NULL;
    status = pRtlAddGrowableFunctionTable( &growable_table, runtime_func, 0, 2, (ULONG_PTR)code_mem,
            (ULONG_PTR)code_mem + code_offset + 64 );
    ok(!status, "RtlAddGrowableFunctionTable failed for runtime_func = %p (aligned), %#x.\n", runtime_func, status );
    ok(growable_table != 0, "Unexpected table value.\n");

    /* Current count is 0. */
    func = pRtlLookupFunctionEntry( (ULONG_PTR)code_mem + code_offset + 8, &base, NULL );
    ok( func == NULL,
        "RtlLookupFunctionEntry didn't return expected function, expected: %p, got: %p\n", runtime_func, func );

    pRtlGrowFunctionTable( growable_table, 1 );

    base = 0xdeadbeef;
    func = pRtlLookupFunctionEntry( (ULONG_PTR)code_mem + code_offset + 8, &base, NULL );
    ok( func == runtime_func,
        "RtlLookupFunctionEntry didn't return expected function, expected: %p, got: %p\n", runtime_func, func );
    ok( base == (ULONG_PTR)code_mem,
        "RtlLookupFunctionEntry returned invalid base, expected: %lx, got: %lx\n", (ULONG_PTR)code_mem, base );

    /* Second function is inaccessible yet. */
    base = 0xdeadbeef;
    func = pRtlLookupFunctionEntry( (ULONG_PTR)code_mem + code_offset + 16, &base, NULL );
    ok( func == NULL,
        "RtlLookupFunctionEntry didn't return expected function, expected: %p, got: %p\n", runtime_func, func );

    pRtlGrowFunctionTable( growable_table, 2 );

    base = 0xdeadbeef;
    func = pRtlLookupFunctionEntry( (ULONG_PTR)code_mem + code_offset + 16, &base, NULL );
    ok( func == runtime_func + 1,
        "RtlLookupFunctionEntry didn't return expected function, expected: %p, got: %p\n", runtime_func, func );
    ok( base == (ULONG_PTR)code_mem,
        "RtlLookupFunctionEntry returned invalid base, expected: %lx, got: %lx\n", (ULONG_PTR)code_mem, base );

    pRtlDeleteGrowableFunctionTable( growable_table );
}

static int termination_handler_called;
static void WINAPI termination_handler(ULONG flags, ULONG64 frame)
{
    termination_handler_called++;

    ok(flags == 1 || broken(flags == 0x401), "flags = %x\n", flags);
    ok(frame == 0x1234, "frame = %p\n", (void*)frame);
}

static void test___C_specific_handler(void)
{
    DISPATCHER_CONTEXT dispatch;
    EXCEPTION_RECORD rec;
    CONTEXT context;
    ULONG64 frame;
    EXCEPTION_DISPOSITION ret;
    SCOPE_TABLE scope_table;

    if (!p__C_specific_handler)
    {
        win_skip("__C_specific_handler not available\n");
        return;
    }

    memset(&rec, 0, sizeof(rec));
    rec.ExceptionFlags = 2; /* EH_UNWINDING */
    frame = 0x1234;
    memset(&dispatch, 0, sizeof(dispatch));
    dispatch.ImageBase = (ULONG_PTR)GetModuleHandleA(NULL);
    dispatch.ControlPc = dispatch.ImageBase + 0x200;
    dispatch.HandlerData = &scope_table;
    dispatch.ContextRecord = &context;
    scope_table.Count = 1;
    scope_table.ScopeRecord[0].BeginAddress = 0x200;
    scope_table.ScopeRecord[0].EndAddress = 0x400;
    scope_table.ScopeRecord[0].HandlerAddress = (ULONG_PTR)termination_handler-dispatch.ImageBase;
    scope_table.ScopeRecord[0].JumpTarget = 0;
    memset(&context, 0, sizeof(context));

    termination_handler_called = 0;
    ret = p__C_specific_handler(&rec, frame, &context, &dispatch);
    ok(ret == ExceptionContinueSearch, "__C_specific_handler returned %x\n", ret);
    ok(termination_handler_called == 1, "termination_handler_called = %d\n",
            termination_handler_called);
    ok(dispatch.ScopeIndex == 1, "dispatch.ScopeIndex = %d\n", dispatch.ScopeIndex);

    ret = p__C_specific_handler(&rec, frame, &context, &dispatch);
    ok(ret == ExceptionContinueSearch, "__C_specific_handler returned %x\n", ret);
    ok(termination_handler_called == 1, "termination_handler_called = %d\n",
            termination_handler_called);
    ok(dispatch.ScopeIndex == 1, "dispatch.ScopeIndex = %d\n", dispatch.ScopeIndex);
}

/* This is heavily based on the i386 exception tests. */
static const struct exception
{
    BYTE     code[40];      /* asm code */
    BYTE     offset;        /* offset of faulting instruction */
    BYTE     length;        /* length of faulting instruction */
    NTSTATUS status;        /* expected status code */
    DWORD    nb_params;     /* expected number of parameters */
    ULONG64  params[4];     /* expected parameters */
    NTSTATUS alt_status;    /* alternative status code */
    DWORD    alt_nb_params; /* alternative number of parameters */
    ULONG64  alt_params[4]; /* alternative parameters */
} exceptions[] =
{
/* 0 */
    /* test some privileged instructions */
    { { 0xfb, 0xc3 },  /* 0: sti; ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x6c, 0xc3 },  /* 1: insb (%dx); ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x6d, 0xc3 },  /* 2: insl (%dx); ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x6e, 0xc3 },  /* 3: outsb (%dx); ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x6f, 0xc3 },  /* 4: outsl (%dx); ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
/* 5 */
    { { 0xe4, 0x11, 0xc3 },  /* 5: inb $0x11,%al; ret */
      0, 2, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xe5, 0x11, 0xc3 },  /* 6: inl $0x11,%eax; ret */
      0, 2, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xe6, 0x11, 0xc3 },  /* 7: outb %al,$0x11; ret */
      0, 2, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xe7, 0x11, 0xc3 },  /* 8: outl %eax,$0x11; ret */
      0, 2, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xed, 0xc3 },  /* 9: inl (%dx),%eax; ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
/* 10 */
    { { 0xee, 0xc3 },  /* 10: outb %al,(%dx); ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xef, 0xc3 },  /* 11: outl %eax,(%dx); ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xf4, 0xc3 },  /* 12: hlt; ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xfa, 0xc3 },  /* 13: cli; ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },

    /* test iret to invalid selector */
    { { 0x6a, 0x00, 0x6a, 0x00, 0x6a, 0x00, 0xcf, 0x83, 0xc4, 0x18, 0xc3 },
      /* 15: pushq $0; pushq $0; pushq $0; iret; addl $24,%esp; ret */
      6, 1, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffffffffffff } },
/* 15 */
    /* test loading an invalid selector */
    { { 0xb8, 0xef, 0xbe, 0x00, 0x00, 0x8e, 0xe8, 0xc3 },  /* 16: mov $beef,%ax; mov %ax,%gs; ret */
      5, 2, STATUS_ACCESS_VIOLATION, 2, { 0, 0xbee8 } }, /* 0xbee8 or 0xffffffff */

    /* test overlong instruction (limit is 15 bytes) */
    { { 0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0xfa,0xc3 },
      0, 16, STATUS_ILLEGAL_INSTRUCTION, 0, { 0 },
      STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffffffffffff } },

    /* test invalid interrupt */
    { { 0xcd, 0xff, 0xc3 },   /* int $0xff; ret */
      0, 2, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffffffffffff } },

    /* test moves to/from Crx */
    { { 0x0f, 0x20, 0xc0, 0xc3 },  /* movl %cr0,%eax; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x20, 0xe0, 0xc3 },  /* movl %cr4,%eax; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },
/* 20 */
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
/* 25 */
    { { 0x0f, 0x23, 0xc0, 0xc3 },  /* movl %eax,%dr0; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x23, 0xc8, 0xc3 },  /* movl %eax,%dr1; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x23, 0xf8, 0xc3 },  /* movl %eax,%dr7; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },

    /* test memory reads */
    { { 0xa1, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc3 },  /* movl 0xfffffffffffffffc,%eax; ret */
      0, 9, STATUS_ACCESS_VIOLATION, 2, { 0, 0xfffffffffffffffc } },
    { { 0xa1, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc3 },  /* movl 0xfffffffffffffffd,%eax; ret */
      0, 9, STATUS_ACCESS_VIOLATION, 2, { 0, 0xfffffffffffffffd } },
/* 30 */
    { { 0xa1, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc3 },  /* movl 0xfffffffffffffffe,%eax; ret */
      0, 9, STATUS_ACCESS_VIOLATION, 2, { 0, 0xfffffffffffffffe } },
    { { 0xa1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc3 },  /* movl 0xffffffffffffffff,%eax; ret */
      0, 9, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffffffffffff } },

    /* test memory writes */
    { { 0xa3, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc3 },  /* movl %eax,0xfffffffffffffffc; ret */
      0, 9, STATUS_ACCESS_VIOLATION, 2, { 1, 0xfffffffffffffffc } },
    { { 0xa3, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc3 },  /* movl %eax,0xfffffffffffffffd; ret */
      0, 9, STATUS_ACCESS_VIOLATION, 2, { 1, 0xfffffffffffffffd } },
    { { 0xa3, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc3 },  /* movl %eax,0xfffffffffffffffe; ret */
      0, 9, STATUS_ACCESS_VIOLATION, 2, { 1, 0xfffffffffffffffe } },
/* 35 */
    { { 0xa3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc3 },  /* movl %eax,0xffffffffffffffff; ret */
      0, 9, STATUS_ACCESS_VIOLATION, 2, { 1, 0xffffffffffffffff } },

    /* test exception with cleared segment registers */
    { {
        0x8c, 0xc0, /* mov %es,%eax */
        0x50,       /* push %rax */
        0x8c, 0xd8, /* mov %ds,%eax */
        0x50,       /* push %rax */
        0x8c, 0xe0, /* mov %fs,%eax */
        0x50,       /* push %rax */
        0x8c, 0xe8, /* mov %gs,%eax */
        0x50,       /* push %rax */
        0x31, 0xc0, /* xor %eax,%eax */
        0x8e, 0xc0, /* mov %eax,%es */
        0x8e, 0xd8, /* mov %eax,%ds */
#if 0
        /* It is observed that fs/gs base is reset
           on some CPUs when setting the segment value
           even to 0 (regardless of CPU spec
           saying otherwise) and it is not currently
           handled in Wine.
           Disable this part to avoid crashing the test. */
        0x8e, 0xe0, /* mov %eax,%fs */
        0x8e, 0xe8, /* mov %eax,%gs */
#else
        0x90, 0x90, /* nop */
        0x90, 0x90, /* nop */
#endif
        0xfa,       /* cli */
        0x58,       /* pop %rax */
#if 0
        0x8e, 0xe8, /* mov %eax,%gs */
        0x58,       /* pop %rax */
        0x8e, 0xe0, /* mov %eax,%fs */
#else
        0x58,       /* pop %rax */
        0x90, 0x90, /* nop */
        0x90, 0x90, /* nop */
#endif
        0x58,       /* pop %rax */
        0x8e, 0xd8, /* mov %eax,%ds */
        0x58,       /* pop %rax */
        0x8e, 0xc0, /* mov %eax,%es */
        0xc3,       /* retq */
      }, 22, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },

    { { 0xf1, 0x90, 0xc3 },  /* icebp; nop; ret */
      1, 1, STATUS_SINGLE_STEP, 0 },
    { { 0xcd, 0x2c, 0xc3 },
      0, 2, STATUS_ASSERTION_FAILURE, 0 },
};

static int got_exception;

static void run_exception_test(void *handler, const void* context,
                               const void *code, unsigned int code_size,
                               DWORD access)
{
    unsigned char buf[8 + 6 + 8 + 8];
    RUNTIME_FUNCTION runtime_func;
    UNWIND_INFO *unwind = (UNWIND_INFO *)buf;
    void (*func)(void) = code_mem;
    DWORD oldaccess, oldaccess2;

    runtime_func.BeginAddress = 0;
    runtime_func.EndAddress = code_size;
    runtime_func.UnwindData = 0x1000;

    unwind->Version = 1;
    unwind->Flags = UNW_FLAG_EHANDLER;
    unwind->SizeOfProlog = 0;
    unwind->CountOfCodes = 0;
    unwind->FrameRegister = 0;
    unwind->FrameOffset = 0;
    *(ULONG *)&buf[4] = 0x1010;
    *(const void **)&buf[8] = context;

    /* jmp near */
    buf[16] = 0xff;
    buf[17] = 0x25;
    *(ULONG *)&buf[18] = 0;
    *(void **)&buf[22] = handler;

    memcpy((unsigned char *)code_mem + 0x1000, buf, sizeof(buf));
    memcpy(code_mem, code, code_size);
    if(access)
        VirtualProtect(code_mem, code_size, access, &oldaccess);

    pRtlAddFunctionTable(&runtime_func, 1, (ULONG_PTR)code_mem);
    func();
    pRtlDeleteFunctionTable(&runtime_func);

    if(access)
        VirtualProtect(code_mem, code_size, oldaccess, &oldaccess2);
}

static DWORD WINAPI handler( EXCEPTION_RECORD *rec, ULONG64 frame,
                      CONTEXT *context, DISPATCHER_CONTEXT *dispatcher )
{
    const struct exception *except = *(const struct exception **)(dispatcher->HandlerData);
    unsigned int i, parameter_count, entry = except - exceptions;
    USHORT ds, es, fs, gs, ss;

    got_exception++;
    trace( "exception %u: %x flags:%x addr:%p\n",
           entry, rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress );

    ok( rec->ExceptionCode == except->status ||
        (except->alt_status != 0 && rec->ExceptionCode == except->alt_status),
        "%u: Wrong exception code %x/%x\n", entry, rec->ExceptionCode, except->status );
    ok( context->Rip == (DWORD_PTR)code_mem + except->offset,
        "%u: Unexpected eip %#lx/%#lx\n", entry,
        context->Rip, (DWORD_PTR)code_mem + except->offset );
    ok( rec->ExceptionAddress == (char*)context->Rip ||
        (rec->ExceptionCode == STATUS_BREAKPOINT && rec->ExceptionAddress == (char*)context->Rip + 1),
        "%u: Unexpected exception address %p/%p\n", entry,
        rec->ExceptionAddress, (char*)context->Rip );

    __asm__ volatile( "movw %%ds,%0" : "=g" (ds) );
    __asm__ volatile( "movw %%es,%0" : "=g" (es) );
    __asm__ volatile( "movw %%fs,%0" : "=g" (fs) );
    __asm__ volatile( "movw %%gs,%0" : "=g" (gs) );
    __asm__ volatile( "movw %%ss,%0" : "=g" (ss) );
    ok( context->SegDs == ds || !ds, "%u: ds %#x does not match %#x\n", entry, context->SegDs, ds );
    ok( context->SegEs == es || !es, "%u: es %#x does not match %#x\n", entry, context->SegEs, es );
    ok( context->SegFs == fs || !fs, "%u: fs %#x does not match %#x\n", entry, context->SegFs, fs );
    ok( context->SegGs == gs || !gs, "%u: gs %#x does not match %#x\n", entry, context->SegGs, gs );
    ok( context->SegSs == ss, "%u: ss %#x does not match %#x\n", entry, context->SegSs, ss );
    ok( context->SegDs == context->SegSs,
        "%u: ds %#x does not match ss %#x\n", entry, context->SegDs, context->SegSs );
    todo_wine ok( context->SegEs == context->SegSs,
        "%u: es %#x does not match ss %#x\n", entry, context->SegEs, context->SegSs );
    todo_wine ok( context->SegGs == context->SegSs,
        "%u: gs %#x does not match ss %#x\n", entry, context->SegGs, context->SegSs );
    todo_wine ok( context->SegFs && context->SegFs != context->SegSs,
        "%u: got fs %#x\n", entry, context->SegFs );

    if (except->status == STATUS_BREAKPOINT && is_wow64)
        parameter_count = 1;
    else if (except->alt_status == 0 || rec->ExceptionCode != except->alt_status)
        parameter_count = except->nb_params;
    else
        parameter_count = except->alt_nb_params;

    ok( rec->NumberParameters == parameter_count,
        "%u: Unexpected parameter count %u/%u\n", entry, rec->NumberParameters, parameter_count );

    /* Most CPUs (except Intel Core apparently) report a segment limit violation */
    /* instead of page faults for accesses beyond 0xffffffffffffffff */
    if (except->nb_params == 2 && except->params[1] >= 0xfffffffffffffffd)
    {
        if (rec->ExceptionInformation[0] == 0 && rec->ExceptionInformation[1] == 0xffffffffffffffff)
            goto skip_params;
    }

    /* Seems that both 0xbee8 and 0xfffffffffffffffff can be returned in windows */
    if (except->nb_params == 2 && rec->NumberParameters == 2
        && except->params[1] == 0xbee8 && rec->ExceptionInformation[1] == 0xffffffffffffffff
        && except->params[0] == rec->ExceptionInformation[0])
    {
        goto skip_params;
    }

    if (except->alt_status == 0 || rec->ExceptionCode != except->alt_status)
    {
        for (i = 0; i < rec->NumberParameters; i++)
            ok( rec->ExceptionInformation[i] == except->params[i],
                "%u: Wrong parameter %d: %lx/%lx\n",
                entry, i, rec->ExceptionInformation[i], except->params[i] );
    }
    else
    {
        for (i = 0; i < rec->NumberParameters; i++)
            ok( rec->ExceptionInformation[i] == except->alt_params[i],
                "%u: Wrong parameter %d: %lx/%lx\n",
                entry, i, rec->ExceptionInformation[i], except->alt_params[i] );
    }

skip_params:
    /* don't handle exception if it's not the address we expected */
    if (context->Rip != (DWORD_PTR)code_mem + except->offset) return ExceptionContinueSearch;

    context->Rip += except->length;
    return ExceptionContinueExecution;
}

static const BYTE segfault_code[5] =
{
    0x31, 0xc0, /* xor    %eax,%eax */
    0x8f, 0x00, /* popq   (%rax) - cause exception */
    0xc3        /* ret */
};

struct dbgreg_test
{
    ULONG_PTR dr0, dr1, dr2, dr3, dr6, dr7;
};
/* test handling of debug registers */
static DWORD WINAPI dreg_handler( EXCEPTION_RECORD *rec, ULONG64 frame,
                                  CONTEXT *context, DISPATCHER_CONTEXT *dispatcher )
{
    const struct dbgreg_test *test = *(const struct dbgreg_test **)dispatcher->HandlerData;

    context->Rip += 2;	/* Skips the popq (%rax) */
    context->Dr0 = test->dr0;
    context->Dr1 = test->dr1;
    context->Dr2 = test->dr2;
    context->Dr3 = test->dr3;
    context->Dr6 = test->dr6;
    context->Dr7 = test->dr7;
    return ExceptionContinueExecution;
}

#define CHECK_DEBUG_REG(n, m) \
    ok((ctx.Dr##n & m) == test->dr##n, "(%d) failed to set debug register " #n " to %p, got %p\n", \
       test_num, (void *)test->dr##n, (void *)ctx.Dr##n)

static int check_debug_registers(int test_num, const struct dbgreg_test *test)
{
    CONTEXT ctx;
    NTSTATUS status;

    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    status = pNtGetContextThread(GetCurrentThread(), &ctx);
    ok(status == STATUS_SUCCESS, "NtGetContextThread failed with %x\n", status);

    if (!ctx.Dr0 && !ctx.Dr1 && !ctx.Dr2 && !ctx.Dr3 && !ctx.Dr6 && !ctx.Dr7)
    {
        win_skip( "debug registers broken\n" );
        return 0;
    }
    CHECK_DEBUG_REG(0, ~0);
    CHECK_DEBUG_REG(1, ~0);
    CHECK_DEBUG_REG(2, ~0);
    CHECK_DEBUG_REG(3, ~0);
    CHECK_DEBUG_REG(6, 0x0f);
    CHECK_DEBUG_REG(7, ~0xdc00);
    return 1;
}

static const BYTE single_stepcode[] =
{
    0x9c,               /* pushf */
    0x58,               /* pop   %rax */
    0x0d,0,1,0,0,       /* or    $0x100,%eax */
    0x50,               /* push   %rax */
    0x9d,               /* popf    */
    0x35,0,1,0,0,       /* xor    $0x100,%eax */
    0x50,               /* push   %rax */
    0x9d,               /* popf    */
    0x90,
    0xc3
};

/* test the single step exception behaviour */
static DWORD WINAPI single_step_handler( EXCEPTION_RECORD *rec, ULONG64 frame,
                                         CONTEXT *context, DISPATCHER_CONTEXT *dispatcher )
{
    got_exception++;
    ok (!(context->EFlags & 0x100), "eflags has single stepping bit set\n");

    if( got_exception < 3)
        context->EFlags |= 0x100;  /* single step until popf instruction */
    else {
        /* show that the last single step exception on the popf instruction
         * (which removed the TF bit), still is a EXCEPTION_SINGLE_STEP exception */
        ok( rec->ExceptionCode == EXCEPTION_SINGLE_STEP,
            "exception is not EXCEPTION_SINGLE_STEP: %x\n", rec->ExceptionCode);
    }
    return ExceptionContinueExecution;
}

/* Test the alignment check (AC) flag handling. */
static const BYTE align_check_code[] =
{
    0x55,               /* push   %rbp */
    0x48,0x89,0xe5,     /* mov    %rsp,%rbp */
    0x9c,               /* pushf   */
    0x9c,               /* pushf   */
    0x58,               /* pop    %rax */
    0x0d,0,0,4,0,       /* or     $0x40000,%eax */
    0x50,               /* push   %rax */
    0x9d,               /* popf    */
    0x48,0x89,0xe0,     /* mov %rsp, %rax */
    0x8b,0x40,0x1,      /* mov 0x1(%rax), %eax - cause exception */
    0x9d,               /* popf    */
    0x5d,               /* pop    %rbp */
    0xc3,               /* ret     */
};

static DWORD WINAPI align_check_handler( EXCEPTION_RECORD *rec, ULONG64 frame,
                                         CONTEXT *context, DISPATCHER_CONTEXT *dispatcher )
{
#ifdef __GNUC__
    __asm__ volatile( "pushfq; andl $~0x40000,(%rsp); popfq" );
#endif
    ok (!(context->EFlags & 0x40000), "eflags has AC bit set\n");
    got_exception++;
    return ExceptionContinueExecution;
}

/* Test the direction flag handling. */
static const BYTE direction_flag_code[] =
{
    0xfd,               /* std */
    0xfa,               /* cli - cause exception */
    0xc3,               /* ret     */
};

static DWORD WINAPI direction_flag_handler( EXCEPTION_RECORD *rec, ULONG64 frame,
                                            CONTEXT *context, DISPATCHER_CONTEXT *dispatcher )
{
#ifdef __GNUC__
    ULONG_PTR flags;
    __asm__("pushfq; popq %0; cld" : "=r" (flags) );
    /* older windows versions don't clear DF properly so don't test */
    if (flags & 0x400) trace( "eflags has DF bit set\n" );
#endif
    ok( context->EFlags & 0x400, "context eflags has DF bit cleared\n" );
    got_exception++;
    context->Rip++;  /* skip cli */
    context->EFlags &= ~0x400;  /* make sure it is cleared on return */
    return ExceptionContinueExecution;
}

/* test single stepping over hardware breakpoint */
static const BYTE dummy_code[] = { 0x90, 0x90, 0x90, 0xc3 };  /* nop, nop, nop, ret */

static DWORD WINAPI bpx_handler( EXCEPTION_RECORD *rec, ULONG64 frame,
                                 CONTEXT *context, DISPATCHER_CONTEXT *dispatcher )
{
    got_exception++;
    ok( rec->ExceptionCode == EXCEPTION_SINGLE_STEP,
        "wrong exception code: %x\n", rec->ExceptionCode);

    if(got_exception == 1) {
        /* hw bp exception on first nop */
        ok( (void *)context->Rip == code_mem, "rip is wrong: %p instead of %p\n",
            (void *)context->Rip, code_mem );
        ok( (context->Dr6 & 0xf) == 1, "B0 flag is not set in Dr6\n");
        ok( !(context->Dr6 & 0x4000), "BS flag is set in Dr6\n");
        context->Dr0 = context->Dr0 + 1;  /* set hw bp again on next instruction */
        context->EFlags |= 0x100;       /* enable single stepping */
    } else if (got_exception == 2) {
        /* single step exception on second nop */
        ok( (char *)context->Rip == (char *)code_mem + 1, "rip is wrong: %p instead of %p\n",
            (void *)context->Rip, (char *)code_mem + 1);
        ok( (context->Dr6 & 0x4000), "BS flag is not set in Dr6\n");
        context->EFlags |= 0x100;
    } else if( got_exception == 3) {
        /* hw bp exception on second nop */
        ok( (void *)context->Rip == (char *)code_mem + 1, "rip is wrong: %p instead of %p\n",
            (void *)context->Rip, (char *)code_mem + 1);
        ok( (context->Dr6 & 0xf) == 1, "B0 flag is not set in Dr6\n");
        ok( !(context->Dr6 & 0x4000), "BS flag is set in Dr6\n");
        context->Dr0 = 0;       /* clear breakpoint */
        context->EFlags |= 0x100;
    } else {
        /* single step exception on third nop */
        ok( (void *)context->Rip == (char *)code_mem + 2, "rip is wrong: %p instead of %p\n",
            (void *)context->Rip, (char *)code_mem + 2);
        ok( (context->Dr6 & 0xf) == 0, "B0...3 flags in Dr6 shouldn't be set\n");
        ok( (context->Dr6 & 0x4000), "BS flag is not set in Dr6\n");
    }

    context->Dr6 = 0;  /* clear status register */
    return ExceptionContinueExecution;
}

/* test int3 handling */
static const BYTE int3_code[] = { 0xcc, 0xc3 };  /* int 3, ret */

static DWORD WINAPI int3_handler( EXCEPTION_RECORD *rec, ULONG64 frame,
                                  CONTEXT *context, DISPATCHER_CONTEXT *dispatcher )
{
    ok( rec->ExceptionAddress == code_mem, "exception address not at: %p, but at %p\n",
                                           code_mem,  rec->ExceptionAddress);
    ok( (void *)context->Rip == code_mem, "rip not at: %p, but at %p\n", code_mem, (void *)context->Rip);
    if ((void *)context->Rip == code_mem) context->Rip++; /* skip breakpoint */

    return ExceptionContinueExecution;
}

/* trap unhandled exceptions */
static LONG CALLBACK exc_filter( EXCEPTION_POINTERS *ptrs )
{
    printf( "%04x unhandled exception %08x at %p rip %p eflags %x\n",
            GetCurrentProcessId(),
            ptrs->ExceptionRecord->ExceptionCode, ptrs->ExceptionRecord->ExceptionAddress,
            (void *)ptrs->ContextRecord->Rip, ptrs->ContextRecord->EFlags );
    fflush( stdout );
    return EXCEPTION_EXECUTE_HANDLER;
}

static void test_exceptions(void)
{
    CONTEXT ctx;
    NTSTATUS res;
    USHORT ds, es, fs, gs, ss;
    struct dbgreg_test dreg_test;

    /* test handling of debug registers */
    memset(&dreg_test, 0, sizeof(dreg_test));

    dreg_test.dr0 = 0x42424240;
    dreg_test.dr2 = 0x126bb070;
    dreg_test.dr3 = 0x0badbad0;
    dreg_test.dr7 = 0xffff0115;
    run_exception_test(dreg_handler, &dreg_test, &segfault_code, sizeof(segfault_code), 0);
    if (check_debug_registers(1, &dreg_test))
    {
        dreg_test.dr0 = 0x42424242;
        dreg_test.dr2 = 0x100f0fe7;
        dreg_test.dr3 = 0x0abebabe;
        dreg_test.dr7 = 0x115;
        run_exception_test(dreg_handler, &dreg_test, &segfault_code, sizeof(segfault_code), 0);
        check_debug_registers(2, &dreg_test);

        /* test single stepping over hardware breakpoint */
        memset(&ctx, 0, sizeof(ctx));
        ctx.Dr0 = (ULONG_PTR)code_mem;  /* set hw bp on first nop */
        ctx.Dr7 = 1;
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        res = pNtSetContextThread( GetCurrentThread(), &ctx );
        ok( res == STATUS_SUCCESS, "NtSetContextThread failed with %x\n", res);

        got_exception = 0;
        run_exception_test(bpx_handler, NULL, dummy_code, sizeof(dummy_code), 0);
        ok( got_exception == 4,"expected 4 exceptions, got %d\n", got_exception);
    }

    /* test single stepping behavior */
    SetUnhandledExceptionFilter( exc_filter );
    got_exception = 0;
    run_exception_test(single_step_handler, NULL, &single_stepcode, sizeof(single_stepcode), 0);
    ok(got_exception == 3, "expected 3 single step exceptions, got %d\n", got_exception);

    /* test alignment exceptions */
    if (0)  /* broken on Windows */
    {
    got_exception = 0;
    run_exception_test(align_check_handler, NULL, align_check_code, sizeof(align_check_code), 0);
    ok(got_exception == 0, "got %d alignment faults, expected 0\n", got_exception);
    }

    /* test direction flag */
    got_exception = 0;
    run_exception_test(direction_flag_handler, NULL, direction_flag_code, sizeof(direction_flag_code), 0);
    ok(got_exception == 1, "got %d exceptions, expected 1\n", got_exception);

    /* test int3 handling */
    run_exception_test(int3_handler, NULL, int3_code, sizeof(int3_code), 0);

    /* test segment registers */
    ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_SEGMENTS;
    res = pNtGetContextThread( GetCurrentThread(), &ctx );
    ok( res == STATUS_SUCCESS, "NtGetContextThread failed with %x\n", res );
    __asm__ volatile( "movw %%ds,%0" : "=g" (ds) );
    __asm__ volatile( "movw %%es,%0" : "=g" (es) );
    __asm__ volatile( "movw %%fs,%0" : "=g" (fs) );
    __asm__ volatile( "movw %%gs,%0" : "=g" (gs) );
    __asm__ volatile( "movw %%ss,%0" : "=g" (ss) );
    ok( ctx.SegDs == ds, "wrong ds %04x / %04x\n", ctx.SegDs, ds );
    ok( ctx.SegEs == es, "wrong es %04x / %04x\n", ctx.SegEs, es );
    ok( ctx.SegFs == fs, "wrong fs %04x / %04x\n", ctx.SegFs, fs );
    ok( ctx.SegGs == gs, "wrong gs %04x / %04x\n", ctx.SegGs, gs );
    ok( ctx.SegSs == ss, "wrong ss %04x / %04x\n", ctx.SegSs, ss );
    todo_wine ok( ctx.SegDs == ctx.SegSs, "wrong ds %04x / %04x\n", ctx.SegDs, ctx.SegSs );
    todo_wine ok( ctx.SegEs == ctx.SegSs, "wrong es %04x / %04x\n", ctx.SegEs, ctx.SegSs );
    ok( ctx.SegFs != ctx.SegSs, "wrong fs %04x / %04x\n", ctx.SegFs, ctx.SegSs );
    todo_wine ok( ctx.SegGs == ctx.SegSs, "wrong gs %04x / %04x\n", ctx.SegGs, ctx.SegSs );
    if (ctx.SegDs == ctx.SegSs)  /* FIXME: remove once Wine is fixed */
    {
    ctx.SegDs = 0;
    ctx.SegEs = ctx.SegFs;
    ctx.SegFs = ctx.SegSs;
    res = pNtSetContextThread( GetCurrentThread(), &ctx );
    ok( res == STATUS_SUCCESS, "NtGetContextThread failed with %x\n", res );
    __asm__ volatile( "movw %%ds,%0" : "=g" (ds) );
    __asm__ volatile( "movw %%es,%0" : "=g" (es) );
    __asm__ volatile( "movw %%fs,%0" : "=g" (fs) );
    __asm__ volatile( "movw %%gs,%0" : "=g" (gs) );
    __asm__ volatile( "movw %%ss,%0" : "=g" (ss) );
    res = pNtGetContextThread( GetCurrentThread(), &ctx );
    ok( res == STATUS_SUCCESS, "NtGetContextThread failed with %x\n", res );
    ok( ctx.SegDs == ds, "wrong ds %08x / %08x\n", ctx.SegDs, ds );
    ok( ctx.SegEs == es, "wrong es %04x / %04x\n", ctx.SegEs, es );
    ok( ctx.SegFs == fs, "wrong fs %04x / %04x\n", ctx.SegFs, fs );
    ok( ctx.SegGs == gs, "wrong gs %04x / %04x\n", ctx.SegGs, gs );
    ok( ctx.SegSs == ss, "wrong ss %04x / %04x\n", ctx.SegSs, ss );
    ok( ctx.SegDs == ctx.SegSs, "wrong ds %04x / %04x\n", ctx.SegDs, ctx.SegSs );
    ok( ctx.SegEs == ctx.SegSs, "wrong es %04x / %04x\n", ctx.SegEs, ctx.SegSs );
    ok( ctx.SegFs != ctx.SegSs, "wrong fs %04x / %04x\n", ctx.SegFs, ctx.SegSs );
    todo_wine ok( ctx.SegGs == ctx.SegSs, "wrong gs %04x / %04x\n", ctx.SegGs, ctx.SegSs );
    }
}

static DWORD WINAPI simd_fault_handler( EXCEPTION_RECORD *rec, ULONG64 frame,
                                        CONTEXT *context, DISPATCHER_CONTEXT *dispatcher )
{
    int *stage = *(int **)dispatcher->HandlerData;

    got_exception++;

    if (*stage == 1)
    {
        /* fault while executing sse instruction */
        context->Rip += 3; /* skip addps */
        return ExceptionContinueExecution;
    }
    else if (*stage == 2 || *stage == 3 )
    {
        /* stage 2 - divide by zero fault */
        /* stage 3 - invalid operation fault */
        if( rec->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION)
            skip("system doesn't support SIMD exceptions\n");
        else
        {
            ULONG expect = *stage == 2 ? EXCEPTION_FLT_DIVIDE_BY_ZERO : EXCEPTION_FLT_INVALID_OPERATION;
            ok( rec->ExceptionCode == expect, "exception code: %#x, should be %#x\n",
                rec->ExceptionCode, expect );
            todo_wine
            ok( rec->NumberParameters == 2, "# of params: %i, should be 2\n", rec->NumberParameters);
            if (rec->NumberParameters == 2)
            {
                /* no idea what these mean */
                ok( rec->ExceptionInformation[0] == 0, "param #0: %lx\n", rec->ExceptionInformation[0]);
                ok( rec->ExceptionInformation[1] != 0, "param #1: %lx\n", rec->ExceptionInformation[1]);
            }
        }
        context->Rip += 3; /* skip divps */
    }
    else
        ok(FALSE, "unexpected stage %x\n", *stage);

    return ExceptionContinueExecution;
}

static const BYTE simd_exception_test[] =
{
    0x48, 0x83, 0xec, 0x8,               /* sub $0x8, %rsp       */
    0x0f, 0xae, 0x1c, 0x24,              /* stmxcsr (%rsp)       */
    0x8b, 0x04, 0x24,                    /* mov    (%rsp),%eax   * store mxcsr */
    0x66, 0x81, 0x24, 0x24, 0xff, 0xfd,  /* andw $0xfdff,(%rsp)  * enable divide by */
    0x0f, 0xae, 0x14, 0x24,              /* ldmxcsr (%rsp)       * zero exceptions  */
    0xb9, 0x01, 0x00, 0x00, 0x00,        /* movl   $0x1,%ecx     */
    0x66, 0x48, 0x0f, 0x6e, 0xc9,        /* movq   %rcx,%xmm1    * fill dividend  */
    0x0f, 0x57, 0xc0,                    /* xorps  %xmm0,%xmm0   * clear divisor  */
    0x0f, 0x5e, 0xc8,                    /* divps  %xmm0,%xmm1   * generate fault */
    0x89, 0x04, 0x24,                    /* mov    %eax,(%rsp)   * restore to old mxcsr */
    0x0f, 0xae, 0x14, 0x24,              /* ldmxcsr (%rsp)       */
    0x48, 0x83, 0xc4, 0x08,              /* add    $0x8,%rsp     */
    0xc3,                                /* ret */
};

static const BYTE simd_exception_test2[] =
{
    0x48, 0x83, 0xec, 0x8,               /* sub $0x8, %rsp       */
    0x0f, 0xae, 0x1c, 0x24,              /* stmxcsr (%rsp)       */
    0x8b, 0x04, 0x24,                    /* mov    (%rsp),%eax   * store mxcsr */
    0x66, 0x81, 0x24, 0x24, 0x7f, 0xff,  /* andw $0xff7f,(%rsp)  * enable invalid       */
    0x0f, 0xae, 0x14, 0x24,              /* ldmxcsr (%rsp)       * operation exceptions */
    0x0f, 0x57, 0xc9,                    /* xorps  %xmm1,%xmm1   * clear dividend */
    0x0f, 0x57, 0xc0,                    /* xorps  %xmm0,%xmm0   * clear divisor  */
    0x0f, 0x5e, 0xc8,                    /* divps  %xmm0,%xmm1   * generate fault */
    0x89, 0x04, 0x24,                    /* mov    %eax,(%rsp)   * restore to old mxcsr */
    0x0f, 0xae, 0x14, 0x24,              /* ldmxcsr (%rsp)       */
    0x48, 0x83, 0xc4, 0x08,              /* add    $0x8,%rsp     */
    0xc3,                                /* ret */
};

static const BYTE sse_check[] =
{
    0x0f, 0x58, 0xc8,                    /* addps  %xmm0,%xmm1 */
    0xc3,                                /* ret */
};

static void test_simd_exceptions(void)
{
    int stage;

    /* test if CPU & OS can do sse */
    stage = 1;
    got_exception = 0;
    run_exception_test(simd_fault_handler, &stage, sse_check, sizeof(sse_check), 0);
    if(got_exception) {
        skip("system doesn't support SSE\n");
        return;
    }

    /* generate a SIMD exception */
    stage = 2;
    got_exception = 0;
    run_exception_test(simd_fault_handler, &stage, simd_exception_test,
                       sizeof(simd_exception_test), 0);
    ok(got_exception == 1, "got exception: %i, should be 1\n", got_exception);

    /* generate a SIMD exception, test FPE_FLTINV */
    stage = 3;
    got_exception = 0;
    run_exception_test(simd_fault_handler, &stage, simd_exception_test2,
                       sizeof(simd_exception_test2), 0);
    ok(got_exception == 1, "got exception: %i, should be 1\n", got_exception);
}

static void test_prot_fault(void)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(exceptions); i++)
    {
        got_exception = 0;
        run_exception_test(handler, &exceptions[i], &exceptions[i].code,
                           sizeof(exceptions[i].code), 0);
        ok( got_exception == (exceptions[i].status != 0),
            "%u: bad exception count %d\n", i, got_exception );
    }
}

static LONG CALLBACK dpe_handler(EXCEPTION_POINTERS *info)
{
    EXCEPTION_RECORD *rec = info->ExceptionRecord;
    DWORD old_prot;

    trace("vect. handler %08x addr:%p\n", rec->ExceptionCode, rec->ExceptionAddress);

    got_exception++;

    ok(rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION,
        "got %#x\n", rec->ExceptionCode);
    ok(rec->NumberParameters == 2, "got %u params\n", rec->NumberParameters);
    ok(rec->ExceptionInformation[0] == EXCEPTION_EXECUTE_FAULT,
        "got %#lx\n", rec->ExceptionInformation[0]);
    ok((void *)rec->ExceptionInformation[1] == code_mem,
        "got %p\n", (void *)rec->ExceptionInformation[1]);

    VirtualProtect(code_mem, 1, PAGE_EXECUTE_READWRITE, &old_prot);

    return EXCEPTION_CONTINUE_EXECUTION;
}

static void test_dpe_exceptions(void)
{
    static const BYTE ret[] = {0xc3};
    DWORD (CDECL *func)(void) = code_mem;
    DWORD old_prot, val = 0, len = 0xdeadbeef;
    NTSTATUS status;
    void *handler;

    status = pNtQueryInformationProcess( GetCurrentProcess(), ProcessExecuteFlags, &val, sizeof val, &len );
    ok( status == STATUS_SUCCESS || status == STATUS_INVALID_PARAMETER, "got status %08x\n", status );
    if (!status)
    {
        ok( len == sizeof(val), "wrong len %u\n", len );
        ok( val == (MEM_EXECUTE_OPTION_DISABLE | MEM_EXECUTE_OPTION_PERMANENT |
                    MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION),
            "wrong val %08x\n", val );
    }
    else ok( len == 0xdeadbeef, "wrong len %u\n", len );

    val = MEM_EXECUTE_OPTION_DISABLE;
    status = pNtSetInformationProcess( GetCurrentProcess(), ProcessExecuteFlags, &val, sizeof val );
    ok( status == STATUS_INVALID_PARAMETER, "got status %08x\n", status );

    memcpy(code_mem, ret, sizeof(ret));

    handler = pRtlAddVectoredExceptionHandler(TRUE, &dpe_handler);
    ok(!!handler, "RtlAddVectoredExceptionHandler failed\n");

    VirtualProtect(code_mem, 1, PAGE_NOACCESS, &old_prot);

    got_exception = 0;
    func();
    ok(got_exception == 1, "got %u exceptions\n", got_exception);

    VirtualProtect(code_mem, 1, old_prot, &old_prot);

    VirtualProtect(code_mem, 1, PAGE_READWRITE, &old_prot);

    got_exception = 0;
    func();
    ok(got_exception == 1, "got %u exceptions\n", got_exception);

    VirtualProtect(code_mem, 1, old_prot, &old_prot);

    pRtlRemoveVectoredExceptionHandler(handler);
}

static const BYTE call_one_arg_code[] = {
    0x55, /* push %rbp */
    0x48, 0x89, 0xe5, /* mov %rsp,%rbp */
    0x48, 0x83, 0xec, 0x20, /* sub $0x20,%rsp */
    0x48, 0x89, 0xc8, /* mov %rcx,%rax */
    0x48, 0x89, 0xd1, /* mov %rdx,%rcx */
    0xff, 0xd0, /* callq *%rax */
    0x90, /* nop */
    0x90, /* nop */
    0x90, /* nop */
    0x90, /* nop */
    0x48, 0x83, 0xc4, 0x20, /* add $0x20,%rsp */
    0x5d, /* pop %rbp */
    0xc3, /* retq  */
};

static int rtlraiseexception_unhandled_handler_called;
static int rtlraiseexception_handler_called;

static void rtlraiseexception_handler_( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                        CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher,
                                        BOOL unhandled_handler )
{
    if (unhandled_handler) rtlraiseexception_unhandled_handler_called = 1;
    else rtlraiseexception_handler_called = 1;

    trace( "exception: %08x flags:%x addr:%p context: Rip:%p\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress, (void *)context->Rip );

    ok( rec->ExceptionAddress == (char *)code_mem + 0x10
        || broken( rec->ExceptionAddress == code_mem || !rec->ExceptionAddress ) /* 2008 */,
        "ExceptionAddress at %p instead of %p\n", rec->ExceptionAddress, (char *)code_mem + 0x10 );

    ok( context->ContextFlags == CONTEXT_ALL || context->ContextFlags == (CONTEXT_ALL | CONTEXT_XSTATE)
        || context->ContextFlags == (CONTEXT_FULL | CONTEXT_SEGMENTS)
        || context->ContextFlags == (CONTEXT_FULL | CONTEXT_SEGMENTS | CONTEXT_XSTATE),
        "wrong context flags %x\n", context->ContextFlags );

    /* check that pc is fixed up only for EXCEPTION_BREAKPOINT
     * even if raised by RtlRaiseException
     */
    if (rec->ExceptionCode == EXCEPTION_BREAKPOINT && test_stage)
        ok( context->Rip == (UINT_PTR)code_mem + 0xf,
            "%d: Rip at %Ix instead of %Ix\n", test_stage, context->Rip, (UINT_PTR)code_mem + 0xf );
    else
        ok( context->Rip == (UINT_PTR)code_mem + 0x10,
            "%d: Rip at %Ix instead of %Ix\n", test_stage, context->Rip, (UINT_PTR)code_mem + 0x10 );

    if (have_vectored_api) ok( context->Rax == 0xf00f00f0, "context->Rax is %Ix, should have been set to 0xf00f00f0 in vectored handler\n", context->Rax );

    /* give the debugger a chance to examine the state a second time */
    /* without the exception handler changing pc */
    if (test_stage == 2)
        return;

    /* pc in context is decreased by 1
     * Increase it again, else execution will continue in the middle of an instruction */
    if (rec->ExceptionCode == EXCEPTION_BREAKPOINT && (context->Rip == (UINT_PTR)code_mem + 0xf))
        context->Rip++;
}

static LONG CALLBACK rtlraiseexception_unhandled_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT context = ExceptionInfo->ContextRecord;
    PEXCEPTION_RECORD rec = ExceptionInfo->ExceptionRecord;
    rtlraiseexception_handler_(rec, NULL, context, NULL, TRUE);
    if (test_stage == 2) return EXCEPTION_CONTINUE_SEARCH;
    return EXCEPTION_CONTINUE_EXECUTION;
}

static DWORD rtlraiseexception_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                        CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    rtlraiseexception_handler_(rec, frame, context, dispatcher, FALSE);
    if (test_stage == 2) return ExceptionContinueSearch;
    return ExceptionContinueExecution;
}

static LONG CALLBACK rtlraiseexception_vectored_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT context = ExceptionInfo->ContextRecord;
    PEXCEPTION_RECORD rec = ExceptionInfo->ExceptionRecord;

    trace( "vect. handler %08x addr:%p Rip:%p\n", rec->ExceptionCode, rec->ExceptionAddress, (void *)context->Rip );

    ok( rec->ExceptionAddress == (char *)code_mem + 0x10
        || broken(rec->ExceptionAddress == code_mem || !rec->ExceptionAddress ) /* 2008 */,
        "ExceptionAddress at %p instead of %p\n", rec->ExceptionAddress, (char *)code_mem + 0x10 );

    /* check that Rip is fixed up only for EXCEPTION_BREAKPOINT
     * even if raised by RtlRaiseException
     */
    if (rec->ExceptionCode == EXCEPTION_BREAKPOINT && test_stage)
        ok( context->Rip == (UINT_PTR)code_mem + 0xf,
            "%d: Rip at %Ix instead of %Ix\n", test_stage, context->Rip, (UINT_PTR)code_mem + 0xf );
    else
        ok( context->Rip == (UINT_PTR)code_mem + 0x10,
            "%d: Rip at %Ix instead of %Ix\n", test_stage, context->Rip, (UINT_PTR)code_mem + 0x10 );

    /* test if context change is preserved from vectored handler to stack handlers */
    context->Rax = 0xf00f00f0;

    return EXCEPTION_CONTINUE_SEARCH;
}

static void run_rtlraiseexception_test(DWORD exceptioncode)
{
    EXCEPTION_REGISTRATION_RECORD frame;
    EXCEPTION_RECORD record;
    PVOID vectored_handler = NULL;

    void (CDECL *func)(void* function, EXCEPTION_RECORD* record) = code_mem;

    record.ExceptionCode = exceptioncode;
    record.ExceptionFlags = 0;
    record.ExceptionRecord = NULL;
    record.ExceptionAddress = NULL; /* does not matter, copied return address */
    record.NumberParameters = 0;

    frame.Handler = rtlraiseexception_handler;
    frame.Prev = NtCurrentTeb()->Tib.ExceptionList;

    memcpy(code_mem, call_one_arg_code, sizeof(call_one_arg_code));

    NtCurrentTeb()->Tib.ExceptionList = &frame;
    if (have_vectored_api)
    {
        vectored_handler = pRtlAddVectoredExceptionHandler(TRUE, &rtlraiseexception_vectored_handler);
        ok(vectored_handler != 0, "RtlAddVectoredExceptionHandler failed\n");
    }
    if (pRtlSetUnhandledExceptionFilter) pRtlSetUnhandledExceptionFilter(&rtlraiseexception_unhandled_handler);

    rtlraiseexception_handler_called = 0;
    rtlraiseexception_unhandled_handler_called = 0;
    func(pRtlRaiseException, &record);
    ok( record.ExceptionAddress == (char *)code_mem + 0x10,
        "address set to %p instead of %p\n", record.ExceptionAddress, (char *)code_mem + 0x10 );

    todo_wine
    ok( !rtlraiseexception_handler_called, "Frame handler called\n" );
    todo_wine_if (test_stage != 2)
    ok( rtlraiseexception_unhandled_handler_called, "UnhandledExceptionFilter wasn't called\n" );

    if (have_vectored_api)
        pRtlRemoveVectoredExceptionHandler(vectored_handler);

    if (pRtlSetUnhandledExceptionFilter) pRtlSetUnhandledExceptionFilter(NULL);
    NtCurrentTeb()->Tib.ExceptionList = frame.Prev;
}

static void test_rtlraiseexception(void)
{
    if (!pRtlRaiseException)
    {
        skip("RtlRaiseException not found\n");
        return;
    }

    /* test without debugger */
    run_rtlraiseexception_test(0x12345);
    run_rtlraiseexception_test(EXCEPTION_BREAKPOINT);
    run_rtlraiseexception_test(EXCEPTION_INVALID_HANDLE);
}

static void test_debugger(DWORD cont_status)
{
    char cmdline[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { 0 };
    DEBUG_EVENT de;
    DWORD continuestatus;
    PVOID code_mem_address = NULL;
    NTSTATUS status;
    SIZE_T size_read;
    BOOL ret;
    int counter = 0;
    si.cb = sizeof(si);

    if(!pNtGetContextThread || !pNtSetContextThread || !pNtReadVirtualMemory || !pNtTerminateProcess)
    {
        skip("NtGetContextThread, NtSetContextThread, NtReadVirtualMemory or NtTerminateProcess not found\n");
        return;
    }

    sprintf(cmdline, "%s %s %s %p", my_argv[0], my_argv[1], "debuggee", &test_stage);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, NULL, &si, &pi);
    ok(ret, "could not create child process error: %u\n", GetLastError());
    if (!ret)
        return;

    do
    {
        continuestatus = cont_status;
        ok(WaitForDebugEvent(&de, INFINITE), "reading debug event\n");

        ret = ContinueDebugEvent(de.dwProcessId, de.dwThreadId, 0xdeadbeef);
        ok(!ret, "ContinueDebugEvent unexpectedly succeeded\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "Unexpected last error: %u\n", GetLastError());

        if (de.dwThreadId != pi.dwThreadId)
        {
            trace("event %d not coming from main thread, ignoring\n", de.dwDebugEventCode);
            ContinueDebugEvent(de.dwProcessId, de.dwThreadId, cont_status);
            continue;
        }

        if (de.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
        {
            if(de.u.CreateProcessInfo.lpBaseOfImage != NtCurrentTeb()->Peb->ImageBaseAddress)
            {
                skip("child process loaded at different address, terminating it\n");
                pNtTerminateProcess(pi.hProcess, 0);
            }
        }
        else if (de.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
        {
            CONTEXT ctx;
            int stage;

            counter++;
            status = pNtReadVirtualMemory(pi.hProcess, &code_mem, &code_mem_address,
                                          sizeof(code_mem_address), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);
            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);

            ctx.ContextFlags = CONTEXT_FULL | CONTEXT_SEGMENTS;
            status = pNtGetContextThread(pi.hThread, &ctx);
            ok(!status, "NtGetContextThread failed with 0x%x\n", status);

            trace("exception 0x%x at %p firstchance=%d Rip=%p, Rax=%p\n",
                  de.u.Exception.ExceptionRecord.ExceptionCode,
                  de.u.Exception.ExceptionRecord.ExceptionAddress,
                  de.u.Exception.dwFirstChance, (char *)ctx.Rip, (char *)ctx.Rax);

            if (counter > 100)
            {
                ok(FALSE, "got way too many exceptions, probably caught in an infinite loop, terminating child\n");
                pNtTerminateProcess(pi.hProcess, 1);
            }
            else if (counter < 2) /* startup breakpoint */
            {
                /* breakpoint is inside ntdll */
                void *ntdll = GetModuleHandleA( "ntdll.dll" );
                IMAGE_NT_HEADERS *nt = RtlImageNtHeader( ntdll );

                ok( (char *)ctx.Rip >= (char *)ntdll &&
                    (char *)ctx.Rip < (char *)ntdll + nt->OptionalHeader.SizeOfImage,
                    "wrong rip %p ntdll %p-%p\n", (void *)ctx.Rip, ntdll,
                    (char *)ntdll + nt->OptionalHeader.SizeOfImage );
            }
            else
            {
                if (stage == 1)
                {
                    ok((char *)ctx.Rip == (char *)code_mem_address + 0x10, "Rip at %p instead of %p\n",
                       (char *)ctx.Rip, (char *)code_mem_address + 0x10);
                    /* setting the context from debugger does not affect the context that the
                     * exception handler gets, except on w2008 */
                    ctx.Rip = (UINT_PTR)code_mem_address + 0x12;
                    ctx.Rax = 0xf00f00f1;
                    /* let the debuggee handle the exception */
                    continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == 2)
                {
                    if (de.u.Exception.dwFirstChance)
                    {
                        ok((char *)ctx.Rip == (char *)code_mem_address + 0x10, "Rip at %p instead of %p\n",
                           (char *)ctx.Rip, (char *)code_mem_address + 0x10);
                        /* setting the context from debugger does not affect the context that the
                         * exception handler gets, except on w2008 */
                        ctx.Rip = (UINT_PTR)code_mem_address + 0x12;
                        ctx.Rax = 0xf00f00f1;
                        /* pass exception to debuggee
                         * exception will not be handled and a second chance exception will be raised */
                        continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                    }
                    else
                    {
                        /* debugger gets context after exception handler has played with it */
                        if (de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
                        {
                            ok((char *)ctx.Rip == (char *)code_mem_address + 0xf, "Rip at %p instead of %p\n",
                               (char *)ctx.Rip, (char *)code_mem_address + 0xf);
                            ctx.Rip += 1;
                        }
                        else ok((char *)ctx.Rip == (char *)code_mem_address + 0x10, "Rip at 0x%x instead of %p\n",
                                ctx.Rip, (char *)code_mem_address + 0x10);
                        /* here we handle exception */
                    }
                }
                else if (stage == 7 || stage == 8)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT,
                       "expected EXCEPTION_BREAKPOINT, got %08x\n", de.u.Exception.ExceptionRecord.ExceptionCode);
                    ok((char *)ctx.Rip == (char *)code_mem_address + 0x30,
                       "expected Rip = %p, got %p\n", (char *)code_mem_address + 0x30, (char *)ctx.Rip);
                    if (stage == 8) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == 9 || stage == 10)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT,
                       "expected EXCEPTION_BREAKPOINT, got %08x\n", de.u.Exception.ExceptionRecord.ExceptionCode);
                    ok((char *)ctx.Rip == (char *)code_mem_address + 2,
                       "expected Rip = %p, got %p\n", (char *)code_mem_address + 2, (char *)ctx.Rip);
                    if (stage == 10) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == 11 || stage == 12 || stage == 13)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_INVALID_HANDLE,
                       "unexpected exception code %08x, expected %08x\n", de.u.Exception.ExceptionRecord.ExceptionCode,
                       EXCEPTION_INVALID_HANDLE);
                    ok(de.u.Exception.ExceptionRecord.NumberParameters == 0,
                       "unexpected number of parameters %d, expected 0\n", de.u.Exception.ExceptionRecord.NumberParameters);

                    if (stage == 12|| stage == 13) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == 14 || stage == 15)
                {
                    test_debugger_xstate(pi.hThread, &ctx, stage);
                }
                else if (stage == 16)
                {
                    USHORT ss;
                    __asm__( "movw %%ss,%0" : "=r" (ss) );
                    ok( ctx.SegSs == ss, "wrong ss %04x / %04x\n", ctx.SegSs, ss );
                    todo_wine ok( ctx.SegDs == ctx.SegSs, "wrong ds %04x / %04x\n", ctx.SegDs, ctx.SegSs );
                    todo_wine ok( ctx.SegEs == ctx.SegSs, "wrong es %04x / %04x\n", ctx.SegEs, ctx.SegSs );
                    ok( ctx.SegFs != ctx.SegSs, "wrong fs %04x / %04x\n", ctx.SegFs, ctx.SegSs );
                    todo_wine ok( ctx.SegGs == ctx.SegSs, "wrong gs %04x / %04x\n", ctx.SegGs, ctx.SegSs );
                    ctx.SegSs = 0;
                    ctx.SegDs = 0;
                    ctx.SegEs = ctx.SegFs;
                    ctx.SegGs = 0;
                    status = pNtSetContextThread( pi.hThread, &ctx );
                    ok( status == STATUS_SUCCESS, "NtSetContextThread failed with %x\n", status );
                    status = pNtGetContextThread( pi.hThread, &ctx );
                    ok( status == STATUS_SUCCESS, "NtGetContextThread failed with %x\n", status );
                    todo_wine ok( ctx.SegSs == ss, "wrong ss %04x / %04x\n", ctx.SegSs, ss );
                    ok( ctx.SegDs == ctx.SegSs, "wrong ds %04x / %04x\n", ctx.SegDs, ctx.SegSs );
                    ok( ctx.SegEs == ctx.SegSs, "wrong es %04x / %04x\n", ctx.SegEs, ctx.SegSs );
                    todo_wine ok( ctx.SegFs != ctx.SegSs, "wrong fs %04x / %04x\n", ctx.SegFs, ctx.SegSs );
                    ok( ctx.SegGs == ctx.SegSs, "wrong gs %04x / %04x\n", ctx.SegGs, ctx.SegSs );
                }
                else
                    ok(FALSE, "unexpected stage %u\n", stage);

                status = pNtSetContextThread(pi.hThread, &ctx);
                ok(!status, "NtSetContextThread failed with 0x%x\n", status);
            }
        }
        else if (de.dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT)
        {
            int stage;
            char buffer[64];

            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);

            ok(!de.u.DebugString.fUnicode, "unexpected unicode debug string event\n");
            ok(de.u.DebugString.nDebugStringLength < sizeof(buffer) - 1, "buffer not large enough to hold %d bytes\n",
               de.u.DebugString.nDebugStringLength);

            memset(buffer, 0, sizeof(buffer));
            status = pNtReadVirtualMemory(pi.hProcess, de.u.DebugString.lpDebugStringData, buffer,
                                          de.u.DebugString.nDebugStringLength, &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);

            if (stage == 3 || stage == 4)
                ok(!strcmp(buffer, "Hello World"), "got unexpected debug string '%s'\n", buffer);
            else /* ignore unrelated debug strings like 'SHIMVIEW: ShimInfo(Complete)' */
                ok(strstr(buffer, "SHIMVIEW") != NULL, "unexpected stage %x, got debug string event '%s'\n", stage, buffer);

            if (stage == 4) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
        }
        else if (de.dwDebugEventCode == RIP_EVENT)
        {
            int stage;

            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);

            if (stage == 5 || stage == 6)
            {
                ok(de.u.RipInfo.dwError == 0x11223344, "got unexpected rip error code %08x, expected %08x\n",
                   de.u.RipInfo.dwError, 0x11223344);
                ok(de.u.RipInfo.dwType  == 0x55667788, "got unexpected rip type %08x, expected %08x\n",
                   de.u.RipInfo.dwType, 0x55667788);
            }
            else
                ok(FALSE, "unexpected stage %x\n", stage);

            if (stage == 6) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
        }

        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, continuestatus);

    } while (de.dwDebugEventCode != EXIT_PROCESS_DEBUG_EVENT);

    wait_child_process( pi.hProcess );
    ret = CloseHandle(pi.hThread);
    ok(ret, "error %u\n", GetLastError());
    ret = CloseHandle(pi.hProcess);
    ok(ret, "error %u\n", GetLastError());
}

static void test_thread_context(void)
{
    CONTEXT context;
    NTSTATUS status;
    int i;
    struct expected
    {
        ULONG64 Rax, Rbx, Rcx, Rdx, Rsi, Rdi, R8, R9, R10, R11,
            R12, R13, R14, R15, Rbp, Rsp, Rip, prev_frame, EFlags;
        ULONG MxCsr;
        XMM_SAVE_AREA32 FltSave;
        WORD SegCs, SegDs, SegEs, SegFs, SegGs, SegSs;
    } expect;
    NTSTATUS (*func_ptr)( void *arg1, void *arg2, struct expected *res, void *func ) = code_mem;

    static const BYTE call_func[] =
    {
        0x55,                                                 /* push   %rbp */
        0x48, 0x89, 0xe5,                                     /* mov    %rsp,%rbp */
        0x48, 0x8d, 0x64, 0x24, 0xd0,                         /* lea    -0x30(%rsp),%rsp */
        0x49, 0x89, 0x00,                                     /* mov    %rax,(%r8) */
        0x49, 0x89, 0x58, 0x08,                               /* mov    %rbx,0x8(%r8) */
        0x49, 0x89, 0x48, 0x10,                               /* mov    %rcx,0x10(%r8) */
        0x49, 0x89, 0x50, 0x18,                               /* mov    %rdx,0x18(%r8) */
        0x49, 0x89, 0x70, 0x20,                               /* mov    %rsi,0x20(%r8) */
        0x49, 0x89, 0x78, 0x28,                               /* mov    %rdi,0x28(%r8) */
        0x4d, 0x89, 0x40, 0x30,                               /* mov    %r8,0x30(%r8) */
        0x4d, 0x89, 0x48, 0x38,                               /* mov    %r9,0x38(%r8) */
        0x4d, 0x89, 0x50, 0x40,                               /* mov    %r10,0x40(%r8) */
        0x4d, 0x89, 0x58, 0x48,                               /* mov    %r11,0x48(%r8) */
        0x4d, 0x89, 0x60, 0x50,                               /* mov    %r12,0x50(%r8) */
        0x4d, 0x89, 0x68, 0x58,                               /* mov    %r13,0x58(%r8) */
        0x4d, 0x89, 0x70, 0x60,                               /* mov    %r14,0x60(%r8) */
        0x4d, 0x89, 0x78, 0x68,                               /* mov    %r15,0x68(%r8) */
        0x49, 0x89, 0x68, 0x70,                               /* mov    %rbp,0x70(%r8) */
        0x49, 0x89, 0x60, 0x78,                               /* mov    %rsp,0x78(%r8) */
        0xff, 0x75, 0x08,                                     /* pushq  0x8(%rbp) */
        0x41, 0x8f, 0x80, 0x80, 0x00, 0x00, 0x00,             /* popq   0x80(%r8) */
        0xff, 0x75, 0x00,                                     /* pushq  0x0(%rbp) */
        0x41, 0x8f, 0x80, 0x88, 0x00, 0x00, 0x00,             /* popq   0x88(%r8) */
        0x9c,                                                 /* pushfq */
        0x41, 0x8f, 0x80, 0x90, 0x00, 0x00, 0x00,             /* popq   0x90(%r8) */
        0x41, 0x0f, 0xae, 0x98, 0x98, 0x00, 0x00, 0x00,       /* stmxcsr 0x98(%r8) */
        0x41, 0x0f, 0xae, 0x80, 0xa0, 0x00, 0x00, 0x00,       /* fxsave 0xa0(%r8) */
        0x66, 0x41, 0x0f, 0x7f, 0x80, 0x40, 0x01, 0x00, 0x00, /* movdqa %xmm0,0x140(%r8) */
        0x66, 0x41, 0x0f, 0x7f, 0x88, 0x50, 0x01, 0x00, 0x00, /* movdqa %xmm1,0x150(%r8) */
        0x66, 0x41, 0x0f, 0x7f, 0x90, 0x60, 0x01, 0x00, 0x00, /* movdqa %xmm2,0x160(%r8) */
        0x66, 0x41, 0x0f, 0x7f, 0x98, 0x70, 0x01, 0x00, 0x00, /* movdqa %xmm3,0x170(%r8) */
        0x66, 0x41, 0x0f, 0x7f, 0xa0, 0x80, 0x01, 0x00, 0x00, /* movdqa %xmm4,0x180(%r8) */
        0x66, 0x41, 0x0f, 0x7f, 0xa8, 0x90, 0x01, 0x00, 0x00, /* movdqa %xmm5,0x190(%r8) */
        0x66, 0x41, 0x0f, 0x7f, 0xb0, 0xa0, 0x01, 0x00, 0x00, /* movdqa %xmm6,0x1a0(%r8) */
        0x66, 0x41, 0x0f, 0x7f, 0xb8, 0xb0, 0x01, 0x00, 0x00, /* movdqa %xmm7,0x1b0(%r8) */
        0x66, 0x45, 0x0f, 0x7f, 0x80, 0xc0, 0x01, 0x00, 0x00, /* movdqa %xmm8,0x1c0(%r8) */
        0x66, 0x45, 0x0f, 0x7f, 0x88, 0xd0, 0x01, 0x00, 0x00, /* movdqa %xmm9,0x1d0(%r8) */
        0x66, 0x45, 0x0f, 0x7f, 0x90, 0xe0, 0x01, 0x00, 0x00, /* movdqa %xmm10,0x1e0(%r8) */
        0x66, 0x45, 0x0f, 0x7f, 0x98, 0xf0, 0x01, 0x00, 0x00, /* movdqa %xmm11,0x1f0(%r8) */
        0x66, 0x45, 0x0f, 0x7f, 0xa0, 0x00, 0x02, 0x00, 0x00, /* movdqa %xmm12,0x200(%r8) */
        0x66, 0x45, 0x0f, 0x7f, 0xa8, 0x10, 0x02, 0x00, 0x00, /* movdqa %xmm13,0x210(%r8) */
        0x66, 0x45, 0x0f, 0x7f, 0xb0, 0x20, 0x02, 0x00, 0x00, /* movdqa %xmm14,0x220(%r8) */
        0x66, 0x45, 0x0f, 0x7f, 0xb8, 0x30, 0x02, 0x00, 0x00, /* movdqa %xmm15,0x230(%r8) */
        0x41, 0x8c, 0x88, 0xa0, 0x02, 0x00, 0x00,             /* mov    %cs,0x2a0(%r8) */
        0x41, 0x8c, 0x98, 0xa2, 0x02, 0x00, 0x00,             /* mov    %ds,0x2a2(%r8) */
        0x41, 0x8c, 0x80, 0xa4, 0x02, 0x00, 0x00,             /* mov    %es,0x2a4(%r8) */
        0x41, 0x8c, 0xa0, 0xa6, 0x02, 0x00, 0x00,             /* mov    %fs,0x2a6(%r8) */
        0x41, 0x8c, 0xa8, 0xa8, 0x02, 0x00, 0x00,             /* mov    %gs,0x2a8(%r8) */
        0x41, 0x8c, 0x90, 0xaa, 0x02, 0x00, 0x00,             /* mov    %ss,0x2aa(%r8) */
        0x41, 0xff, 0xd1,                                     /* callq  *%r9 */
        0xc9,                                                 /* leaveq */
        0xc3,                                                 /* retq */
    };

    memcpy( func_ptr, call_func, sizeof(call_func) );

#define COMPARE(reg) \
    ok( context.reg == expect.reg, "wrong " #reg " %p/%p\n", (void *)(ULONG64)context.reg, (void *)(ULONG64)expect.reg )

    memset( &context, 0xcc, sizeof(context) );
    memset( &expect, 0xcc, sizeof(expect) );
    func_ptr( &context, 0, &expect, pRtlCaptureContext );
    trace( "expect: rax=%p rbx=%p rcx=%p rdx=%p rsi=%p rdi=%p "
           "r8=%p r9=%p r10=%p r11=%p r12=%p r13=%p r14=%p r15=%p "
           "rbp=%p rsp=%p rip=%p cs=%04x ds=%04x es=%04x fs=%04x gs=%04x ss=%04x flags=%08x prev=%08x\n",
           (void *)expect.Rax, (void *)expect.Rbx, (void *)expect.Rcx, (void *)expect.Rdx,
           (void *)expect.Rsi, (void *)expect.Rdi, (void *)expect.R8, (void *)expect.R9,
           (void *)expect.R10, (void *)expect.R11, (void *)expect.R12, (void *)expect.R13,
           (void *)expect.R14, (void *)expect.R15, (void *)expect.Rbp, (void *)expect.Rsp,
           (void *)expect.Rip, expect.SegCs, expect.SegDs, expect.SegEs,
           expect.SegFs, expect.SegGs, expect.SegSs, expect.EFlags, expect.prev_frame );
    trace( "actual: rax=%p rbx=%p rcx=%p rdx=%p rsi=%p rdi=%p "
           "r8=%p r9=%p r10=%p r11=%p r12=%p r13=%p r14=%p r15=%p "
           "rbp=%p rsp=%p rip=%p cs=%04x ds=%04x es=%04x fs=%04x gs=%04x ss=%04x flags=%08x prev=%08x\n",
           (void *)context.Rax, (void *)context.Rbx, (void *)context.Rcx, (void *)context.Rdx,
           (void *)context.Rsi, (void *)context.Rdi, (void *)context.R8, (void *)context.R9,
           (void *)context.R10, (void *)context.R11, (void *)context.R12, (void *)context.R13,
           (void *)context.R14, (void *)context.R15, (void *)context.Rbp, (void *)context.Rsp,
           (void *)context.Rip, context.SegCs, context.SegDs, context.SegEs,
           context.SegFs, context.SegGs, context.SegSs, context.EFlags );

    ok( context.ContextFlags == (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS | CONTEXT_FLOATING_POINT),
        "wrong flags %08x\n", context.ContextFlags );
    COMPARE( Rax );
    COMPARE( Rbx );
    COMPARE( Rcx );
    COMPARE( Rdx );
    COMPARE( Rsi );
    COMPARE( Rdi );
    COMPARE( R8 );
    COMPARE( R9 );
    COMPARE( R10 );
    COMPARE( R11 );
    COMPARE( R12 );
    COMPARE( R13 );
    COMPARE( R14 );
    COMPARE( R15 );
    COMPARE( Rbp );
    COMPARE( Rsp );
    COMPARE( EFlags );
    COMPARE( MxCsr );
    COMPARE( SegCs );
    COMPARE( SegDs );
    COMPARE( SegEs );
    COMPARE( SegFs );
    COMPARE( SegGs );
    COMPARE( SegSs );
    ok( !memcmp( &context.FltSave, &expect.FltSave, offsetof( XMM_SAVE_AREA32, XmmRegisters )),
        "wrong FltSave\n" );
    for (i = 0; i < 16; i++)
        ok( !memcmp( &context.Xmm0 + i, &expect.FltSave.XmmRegisters[i], sizeof(context.Xmm0) ),
            "wrong xmm%u\n", i );
    /* Rip is return address from RtlCaptureContext */
    ok( context.Rip == (ULONG64)func_ptr + sizeof(call_func) - 2,
        "wrong Rip %p/%p\n", (void *)context.Rip, (char *)func_ptr + sizeof(call_func) - 2 );

    memset( &context, 0xcc, sizeof(context) );
    memset( &expect, 0xcc, sizeof(expect) );
    context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS | CONTEXT_FLOATING_POINT;

    status = func_ptr( GetCurrentThread(), &context, &expect, pNtGetContextThread );
    ok( status == STATUS_SUCCESS, "NtGetContextThread failed %08x\n", status );
    trace( "expect: rax=%p rbx=%p rcx=%p rdx=%p rsi=%p rdi=%p "
           "r8=%p r9=%p r10=%p r11=%p r12=%p r13=%p r14=%p r15=%p "
           "rbp=%p rsp=%p rip=%p cs=%04x ds=%04x es=%04x fs=%04x gs=%04x ss=%04x flags=%08x prev=%08x\n",
           (void *)expect.Rax, (void *)expect.Rbx, (void *)expect.Rcx, (void *)expect.Rdx,
           (void *)expect.Rsi, (void *)expect.Rdi, (void *)expect.R8, (void *)expect.R9,
           (void *)expect.R10, (void *)expect.R11, (void *)expect.R12, (void *)expect.R13,
           (void *)expect.R14, (void *)expect.R15, (void *)expect.Rbp, (void *)expect.Rsp,
           (void *)expect.Rip, expect.SegCs, expect.SegDs, expect.SegEs,
           expect.SegFs, expect.SegGs, expect.SegSs, expect.EFlags, expect.prev_frame );
    trace( "actual: rax=%p rbx=%p rcx=%p rdx=%p rsi=%p rdi=%p "
           "r8=%p r9=%p r10=%p r11=%p r12=%p r13=%p r14=%p r15=%p "
           "rbp=%p rsp=%p rip=%p cs=%04x ds=%04x es=%04x fs=%04x gs=%04x ss=%04x flags=%08x prev=%08x\n",
           (void *)context.Rax, (void *)context.Rbx, (void *)context.Rcx, (void *)context.Rdx,
           (void *)context.Rsi, (void *)context.Rdi, (void *)context.R8, (void *)context.R9,
           (void *)context.R10, (void *)context.R11, (void *)context.R12, (void *)context.R13,
           (void *)context.R14, (void *)context.R15, (void *)context.Rbp, (void *)context.Rsp,
           (void *)context.Rip, context.SegCs, context.SegDs, context.SegEs,
           context.SegFs, context.SegGs, context.SegSs, context.EFlags );
    /* other registers are not preserved */
    COMPARE( Rbx );
    COMPARE( Rsi );
    COMPARE( Rdi );
    COMPARE( R12 );
    COMPARE( R13 );
    COMPARE( R14 );
    COMPARE( R15 );
    COMPARE( Rbp );
    COMPARE( MxCsr );
    COMPARE( SegCs );
    COMPARE( SegDs );
    COMPARE( SegEs );
    COMPARE( SegFs );
    COMPARE( SegGs );
    COMPARE( SegSs );

    /* AMD CPUs don't save the opcode or data pointer if no exception is
     * pending; see the AMD64 Architecture Programmer's Manual Volume 5 s.v.
     * FXSAVE */
    memcpy( &expect.FltSave, &context.FltSave, 0x12 );

    ok( !memcmp( &context.FltSave, &expect.FltSave, offsetof( XMM_SAVE_AREA32, ErrorOffset )), "wrong FltSave\n" );
    for (i = 6; i < 16; i++)
        ok( !memcmp( &context.Xmm0 + i, &expect.FltSave.XmmRegisters[i], sizeof(context.Xmm0) ),
            "wrong xmm%u\n", i );
    /* Rsp is the stack upon entry to NtGetContextThread */
    ok( context.Rsp == expect.Rsp - 8,
        "wrong Rsp %p/%p\n", (void *)context.Rsp, (void *)expect.Rsp );
    /* Rip is somewhere close to the NtGetContextThread implementation */
    ok( (char *)context.Rip >= (char *)pNtGetContextThread - 0x40000 &&
        (char *)context.Rip <= (char *)pNtGetContextThread + 0x40000,
        "wrong Rip %p/%p\n", (void *)context.Rip, (void *)pNtGetContextThread );
#undef COMPARE
}

static void test_wow64_context(void)
{
    const char appname[] = "C:\\windows\\syswow64\\cmd.exe";
    char cmdline[256];
    THREAD_BASIC_INFORMATION info;
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = {0};
    WOW64_CONTEXT ctx, *ctx_ptr = NULL;
    CONTEXT context;
    NTSTATUS ret;
    TEB teb;
    TEB32 teb32;
    SIZE_T res, cpu_size = 0;
    WOW64_CPURESERVED *cpu = NULL;
    WOW64_CPU_AREA_INFO cpu_info;
    BOOL r, got32, got64;
    unsigned int i, cs32, cs64;
    ULONG_PTR ecx, rcx;

    memset(&ctx, 0x55, sizeof(ctx));
    ctx.ContextFlags = WOW64_CONTEXT_ALL;
    ret = pRtlWow64GetThreadContext( GetCurrentThread(), &ctx );
    ok(ret == STATUS_INVALID_PARAMETER || broken(ret == STATUS_PARTIAL_COPY), "got %#x\n", ret);

    sprintf( cmdline, "\"%s\" /c for /l %%n in () do @echo >nul", appname );
    r = CreateProcessA( appname, cmdline, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);
    ok( r, "failed to start %s err %u\n", appname, GetLastError() );

    ret = pRtlWow64GetThreadContext( pi.hThread, &ctx );
    ok(ret == STATUS_SUCCESS, "got %#x\n", ret);
    ok(ctx.ContextFlags == WOW64_CONTEXT_ALL, "got context flags %#x\n", ctx.ContextFlags);
    ok(!ctx.Ebp, "got ebp %08x\n", ctx.Ebp);
    ok(!ctx.Ecx, "got ecx %08x\n", ctx.Ecx);
    ok(!ctx.Edx, "got edx %08x\n", ctx.Edx);
    ok(!ctx.Esi, "got esi %08x\n", ctx.Esi);
    ok(!ctx.Edi, "got edi %08x\n", ctx.Edi);
    ok((ctx.EFlags & ~2) == 0x200, "got eflags %08x\n", ctx.EFlags);
    ok((WORD) ctx.FloatSave.ControlWord == 0x27f, "got control word %08x\n",
        ctx.FloatSave.ControlWord);
    ok(*(WORD *)ctx.ExtendedRegisters == 0x27f, "got SSE control word %04x\n",
       *(WORD *)ctx.ExtendedRegisters);

    ret = pRtlWow64SetThreadContext( pi.hThread, &ctx );
    ok(ret == STATUS_SUCCESS, "got %#x\n", ret);

    pNtQueryInformationThread( pi.hThread, ThreadBasicInformation, &info, sizeof(info), NULL );
    if (!ReadProcessMemory( pi.hProcess, info.TebBaseAddress, &teb, sizeof(teb), &res )) res = 0;
    ok( res == sizeof(teb), "wrong len %lx\n", res );

    memset( &teb32, 0, sizeof(teb32) );
    if (teb.WowTebOffset > 1)
    {
        if (!ReadProcessMemory( pi.hProcess, (char *)info.TebBaseAddress + teb.WowTebOffset,
                                &teb32, sizeof(teb32), &res )) res = 0;
        ok( res == sizeof(teb32), "wrong len %lx\n", res );

        ok( ((ctx.Esp + 0xfff) & ~0xfff) == teb32.Tib.StackBase,
            "esp is not at top of stack: %08x / %08x\n", ctx.Esp, teb32.Tib.StackBase );
        ok( ULongToPtr( teb32.Tib.StackBase ) <= teb.DeallocationStack ||
            ULongToPtr( teb32.DeallocationStack ) >= teb.Tib.StackBase,
            "stacks overlap %08x-%08x / %p-%p\n", teb32.DeallocationStack, teb32.Tib.StackBase,
            teb.DeallocationStack, teb.Tib.StackBase );
    }

    if (pRtlWow64GetCpuAreaInfo)
    {
        ok( teb.TlsSlots[WOW64_TLS_CPURESERVED] == teb.Tib.StackBase, "wrong cpu reserved %p / %p\n",
            teb.TlsSlots[WOW64_TLS_CPURESERVED], teb.Tib.StackBase );
        cpu_size = 0x1000 - ((ULONG_PTR)teb.TlsSlots[WOW64_TLS_CPURESERVED] & 0xfff);
        cpu = malloc( cpu_size );
        if (!ReadProcessMemory( pi.hProcess, teb.TlsSlots[WOW64_TLS_CPURESERVED], cpu, cpu_size, &res )) res = 0;
        ok( res == cpu_size, "wrong len %lx\n", res );
        ok( cpu->Machine == IMAGE_FILE_MACHINE_I386, "wrong machine %04x\n", cpu->Machine );
        ret = pRtlWow64GetCpuAreaInfo( cpu, 0, &cpu_info );
        ok( !ret, "RtlWow64GetCpuAreaInfo failed %x\n", ret );
        ctx_ptr = (WOW64_CONTEXT *)cpu_info.Context;
        ok(!*(void **)cpu_info.ContextEx, "got context_ex %p\n", *(void **)cpu_info.ContextEx);
        ok(ctx_ptr->ContextFlags == WOW64_CONTEXT_ALL, "got context flags %#x\n", ctx_ptr->ContextFlags);
        ok(ctx_ptr->Eax == ctx.Eax, "got eax %08x / %08x\n", ctx_ptr->Eax, ctx.Eax);
        ok(ctx_ptr->Ebx == ctx.Ebx, "got ebx %08x / %08x\n", ctx_ptr->Ebx, ctx.Ebx);
        ok(ctx_ptr->Ecx == ctx.Ecx, "got ecx %08x / %08x\n", ctx_ptr->Ecx, ctx.Ecx);
        ok(ctx_ptr->Edx == ctx.Edx, "got edx %08x / %08x\n", ctx_ptr->Edx, ctx.Edx);
        ok(ctx_ptr->Ebp == ctx.Ebp, "got ebp %08x / %08x\n", ctx_ptr->Ebp, ctx.Ebp);
        ok(ctx_ptr->Esi == ctx.Esi, "got esi %08x / %08x\n", ctx_ptr->Esi, ctx.Esi);
        ok(ctx_ptr->Edi == ctx.Edi, "got edi %08x / %08x\n", ctx_ptr->Edi, ctx.Edi);
        ok(ctx_ptr->SegCs == ctx.SegCs, "got cs %04x / %04x\n", ctx_ptr->SegCs, ctx.SegCs);
        ok(ctx_ptr->SegDs == ctx.SegDs, "got ds %04x / %04x\n", ctx_ptr->SegDs, ctx.SegDs);
        ok(ctx_ptr->SegEs == ctx.SegEs, "got es %04x / %04x\n", ctx_ptr->SegEs, ctx.SegEs);
        ok(ctx_ptr->SegFs == ctx.SegFs, "got fs %04x / %04x\n", ctx_ptr->SegFs, ctx.SegFs);
        ok(ctx_ptr->SegGs == ctx.SegGs, "got gs %04x / %04x\n", ctx_ptr->SegGs, ctx.SegGs);
        ok(ctx_ptr->SegSs == ctx.SegSs, "got ss %04x / %04x\n", ctx_ptr->SegSs, ctx.SegSs);
        ok(ctx_ptr->EFlags == ctx.EFlags, "got eflags %08x / %08x\n", ctx_ptr->EFlags, ctx.EFlags);
        ok((WORD)ctx_ptr->FloatSave.ControlWord == ctx.FloatSave.ControlWord,
           "got control word %08x / %08x\n", ctx_ptr->FloatSave.ControlWord, ctx.FloatSave.ControlWord);
        ok(*(WORD *)ctx_ptr->ExtendedRegisters == *(WORD *)ctx.ExtendedRegisters,
           "got SSE control word %04x\n", *(WORD *)ctx_ptr->ExtendedRegisters,
           *(WORD *)ctx.ExtendedRegisters);

        ecx = ctx.Ecx;
        ctx.Ecx = 0x12345678;
        ret = pRtlWow64SetThreadContext( pi.hThread, &ctx );
        ok(ret == STATUS_SUCCESS, "got %#x\n", ret);
        if (!ReadProcessMemory( pi.hProcess, teb.TlsSlots[WOW64_TLS_CPURESERVED], cpu, cpu_size, &res )) res = 0;
        ok( res == cpu_size, "wrong len %lx\n", res );
        todo_wine
        ok( ctx_ptr->Ecx == 0x12345678, "got ecx %08x\n", ctx_ptr->Ecx );
        ctx.Ecx = ecx;
        pRtlWow64SetThreadContext( pi.hThread, &ctx );
    }
    else win_skip( "RtlWow64GetCpuAreaInfo not supported\n" );

    memset( &context, 0x55, sizeof(context) );
    context.ContextFlags = CONTEXT_ALL;
    ret = pNtGetContextThread( pi.hThread, &context );
    ok(ret == STATUS_SUCCESS, "got %#x\n", ret);
    ok( context.ContextFlags == CONTEXT_ALL, "got context flags %#x\n", context.ContextFlags );
    todo_wine
    ok( !context.Rax, "rax is not zero %lx\n", context.Rax );
    todo_wine
    ok( !context.Rbx, "rbx is not zero %lx\n", context.Rbx );
    ok( !context.Rsi, "rsi is not zero %lx\n", context.Rsi );
    ok( !context.Rdi, "rdi is not zero %lx\n", context.Rdi );
    ok( !context.Rbp, "rbp is not zero %lx\n", context.Rbp );
    ok( !context.R8, "r8 is not zero %lx\n", context.R8 );
    ok( !context.R9, "r9 is not zero %lx\n", context.R9 );
    ok( !context.R10, "r10 is not zero %lx\n", context.R10 );
    ok( !context.R11, "r11 is not zero %lx\n", context.R11 );
    ok( !context.R12, "r12 is not zero %lx\n", context.R12 );
    ok( !context.R13, "r13 is not zero %lx\n", context.R13 );
    ok( !context.R14, "r14 is not zero %lx\n", context.R14 );
    ok( !context.R15, "r15 is not zero %lx\n", context.R15 );
    todo_wine
    ok( ((ULONG_PTR)context.Rsp & ~0xfff) == ((ULONG_PTR)teb.Tib.StackBase & ~0xfff),
        "rsp is not at top of stack %p / %p\n", (void *)context.Rsp, teb.Tib.StackBase );
    todo_wine
    ok( context.EFlags == 0x200, "wrong flags %08x\n", context.EFlags );
    ok( context.MxCsr == 0x1f80, "wrong mxcsr %08x\n", context.MxCsr );
    ok( context.FltSave.ControlWord == 0x27f, "wrong control %08x\n", context.FltSave.ControlWord );
    todo_wine
    ok( context.SegCs != ctx.SegCs, "wrong cs %04x\n", context.SegCs );
    ok( context.SegDs == ctx.SegDs, "wrong ds %04x / %04x\n", context.SegDs, ctx.SegDs  );
    ok( context.SegEs == ctx.SegEs, "wrong es %04x / %04x\n", context.SegEs, ctx.SegEs  );
    ok( context.SegFs == ctx.SegFs, "wrong fs %04x / %04x\n", context.SegFs, ctx.SegFs  );
    ok( context.SegGs == ctx.SegGs, "wrong gs %04x / %04x\n", context.SegGs, ctx.SegGs  );
    ok( context.SegSs == ctx.SegSs, "wrong ss %04x / %04x\n", context.SegSs, ctx.SegSs  );

    cs32 = ctx.SegCs;
    cs64 = context.SegCs;
    if (cs32 == cs64)
    {
        todo_wine win_skip( "no wow64 support\n" );
        goto done;
    }

    for (i = 0, got32 = got64 = FALSE; i < 10000 && !(got32 && got64); i++)
    {
        ResumeThread( pi.hThread );
        Sleep( 1 );
        SuspendThread( pi.hThread );
        memset( &context, 0x55, sizeof(context) );
        context.ContextFlags = CONTEXT_ALL;
        ret = pNtGetContextThread( pi.hThread, &context );
        ok( ret == STATUS_SUCCESS, "got %#x\n", ret );
        if (ret) break;
        if (context.SegCs == cs32 && got32) continue;
        if (context.SegCs == cs64 && got64) continue;
        if (context.SegCs != cs32 && context.SegCs != cs64)
        {
            ok( 0, "unexpected cs %04x\n", context.SegCs );
            break;
        }

        memset( &ctx, 0x55, sizeof(ctx) );
        ctx.ContextFlags = WOW64_CONTEXT_ALL;
        ret = pRtlWow64GetThreadContext( pi.hThread, &ctx );
        ok(ret == STATUS_SUCCESS, "got %#x\n", ret);
        ok( ctx.ContextFlags == WOW64_CONTEXT_ALL, "got context flags %#x\n", ctx.ContextFlags );
        if (context.SegCs == cs32)
        {
            trace( "in 32-bit mode %04x\n", context.SegCs );
            ok( ctx.Eip == context.Rip, "cs32: eip %08x / %p\n", ctx.Eip, (void *)context.Rip );
            ok( ctx.Ebp == context.Rbp, "cs32: ebp %08x / %p\n", ctx.Ebp, (void *)context.Rbp );
            ok( ctx.Esp == context.Rsp, "cs32: esp %08x / %p\n", ctx.Esp, (void *)context.Rsp );
            ok( ctx.Eax == context.Rax, "cs32: eax %08x / %p\n", ctx.Eax, (void *)context.Rax );
            ok( ctx.Ebx == context.Rbx, "cs32: ebx %08x / %p\n", ctx.Ebx, (void *)context.Rbx );
            ok( ctx.Ecx == context.Rcx, "cs32: ecx %08x / %p\n", ctx.Ecx, (void *)context.Rcx );
            ok( ctx.Edx == context.Rdx, "cs32: edx %08x / %p\n", ctx.Edx, (void *)context.Rdx );
            ok( ctx.Esi == context.Rsi, "cs32: esi %08x / %p\n", ctx.Esi, (void *)context.Rsi );
            ok( ctx.Edi == context.Rdi, "cs32: edi %08x / %p\n", ctx.Edi, (void *)context.Rdi );
            ok( ctx.SegCs == cs32, "cs32: wrong cs %04x / %04x\n", ctx.SegCs, cs32 );
            ok( ctx.SegDs == context.SegDs, "cs32: wrong ds %04x / %04x\n", ctx.SegDs, context.SegDs );
            ok( ctx.SegEs == context.SegEs, "cs32: wrong es %04x / %04x\n", ctx.SegEs, context.SegEs );
            ok( ctx.SegFs == context.SegFs, "cs32: wrong fs %04x / %04x\n", ctx.SegFs, context.SegFs );
            ok( ctx.SegGs == context.SegGs, "cs32: wrong gs %04x / %04x\n", ctx.SegGs, context.SegGs );
            ok( ctx.SegSs == context.SegSs, "cs32: wrong ss %04x / %04x\n", ctx.SegSs, context.SegSs );
            if (teb32.DeallocationStack)
                ok( ctx.Esp >= teb32.DeallocationStack && ctx.Esp <= teb32.Tib.StackBase,
                    "cs32: esp not inside 32-bit stack %08x / %08x-%08x\n", ctx.Esp,
                    teb32.DeallocationStack, teb32.Tib.StackBase );
            /* r12 points to the TEB */
            ok( (void *)context.R12 == info.TebBaseAddress,
                "cs32: r12 not pointing to the TEB %p / %p\n", (void *)context.R12, info.TebBaseAddress );
            /* r13 points inside the cpu area */
            ok( (void *)context.R13 >= teb.TlsSlots[WOW64_TLS_CPURESERVED] &&
                context.R13 <= ((ULONG_PTR)teb.TlsSlots[WOW64_TLS_CPURESERVED] | 0xfff),
                "cs32: r13 not pointing into cpu area %p / %p\n", (void *)context.R13,
                teb.TlsSlots[WOW64_TLS_CPURESERVED] );
            /* r14 stores the 64-bit stack pointer */
            ok( (void *)context.R14 >= teb.DeallocationStack && (void *)context.R14 <= teb.Tib.StackBase,
                "cs32: r14 not inside 32-bit stack %p / %p-%p\n", (void *)context.R14,
                (void *)teb.DeallocationStack, (void *)teb.Tib.StackBase );

            if (pRtlWow64GetCpuAreaInfo)
            {
                /* in 32-bit mode, the 32-bit context is the current cpu context, not the stored one */
                if (!ReadProcessMemory( pi.hProcess, teb.TlsSlots[WOW64_TLS_CPURESERVED],
                                        cpu, cpu_size, &res )) res = 0;
                ok( res == cpu_size, "wrong len %lx\n", res );
                ok(ctx_ptr->ContextFlags == WOW64_CONTEXT_ALL,
                   "cs32: got context flags %#x\n", ctx_ptr->ContextFlags);
                ok(ctx_ptr->Eip != ctx.Eip, "cs32: got eip %08x / %08x\n", ctx_ptr->Eip, ctx.Eip);

                /* changing either context changes the actual cpu context */
                rcx = context.Rcx;
                ecx = ctx_ptr->Ecx;
                context.Rcx = 0xfedcba987654321ull;
                pNtSetContextThread( pi.hThread, &context );
                memset( &ctx, 0x55, sizeof(ctx) );
                ctx.ContextFlags = WOW64_CONTEXT_ALL;
                pRtlWow64GetThreadContext( pi.hThread, &ctx );
                ok( ctx.Ecx == 0x87654321, "cs64: ecx set to %08x\n", ctx.Ecx );
                ReadProcessMemory( pi.hProcess, teb.TlsSlots[WOW64_TLS_CPURESERVED], cpu, cpu_size, &res );
                ok( ctx_ptr->Ecx == ecx, "cs64: ecx set to %08x\n", ctx_ptr->Ecx );
                ctx.Ecx = 0x33334444;
                pRtlWow64SetThreadContext( pi.hThread, &ctx );
                memset( &ctx, 0x55, sizeof(ctx) );
                ctx.ContextFlags = WOW64_CONTEXT_ALL;
                pRtlWow64GetThreadContext( pi.hThread, &ctx );
                ok( ctx.Ecx == 0x33334444, "cs64: ecx set to %08x\n", ctx.Ecx );
                ReadProcessMemory( pi.hProcess, teb.TlsSlots[WOW64_TLS_CPURESERVED], cpu, cpu_size, &res );
                ok( ctx_ptr->Ecx == ecx, "cs64: ecx set to %08x\n", ctx_ptr->Ecx );
                memset( &context, 0x55, sizeof(context) );
                context.ContextFlags = CONTEXT_ALL;
                pNtGetContextThread( pi.hThread, &context );
                ok( context.Rcx == 0x33334444, "cs64: rcx set to %p\n", (void *)context.Rcx );
                /* restore everything */
                context.Rcx = rcx;
                pNtSetContextThread( pi.hThread, &context );
            }
            got32 = TRUE;
        }
        else
        {
            trace( "in 64-bit mode %04x\n", context.SegCs );
            ok( ctx.Eip != context.Rip, "cs64: eip %08x / %p\n", ctx.Eip, (void *)context.Rip);
            ok( ctx.SegCs == cs32, "cs64: wrong cs %04x / %04x\n", ctx.SegCs, cs32 );
            ok( ctx.SegDs == context.SegDs, "cs64: wrong ds %04x / %04x\n", ctx.SegDs, context.SegDs );
            ok( ctx.SegEs == context.SegEs, "cs64: wrong es %04x / %04x\n", ctx.SegEs, context.SegEs );
            ok( ctx.SegFs == context.SegFs, "cs64: wrong fs %04x / %04x\n", ctx.SegFs, context.SegFs );
            ok( ctx.SegGs == context.SegGs, "cs64: wrong gs %04x / %04x\n", ctx.SegGs, context.SegGs );
            ok( ctx.SegSs == context.SegSs, "cs64: wrong ss %04x / %04x\n", ctx.SegSs, context.SegSs );
            if (teb32.DeallocationStack)
                ok( ctx.Esp >= teb32.DeallocationStack && ctx.Esp <= teb32.Tib.StackBase,
                    "cs64: esp not inside 32-bit stack %08x / %08x-%08x\n", ctx.Esp,
                    teb32.DeallocationStack, teb32.Tib.StackBase );
            ok( (void *)context.Rsp >= teb.DeallocationStack && (void *)context.Rsp <= teb.Tib.StackBase,
                "cs64: rsp not inside 64-bit stack %p / %p-%p\n", (void *)context.Rsp,
                teb.DeallocationStack, teb.Tib.StackBase );

            if (pRtlWow64GetCpuAreaInfo)
            {
                /* in 64-bit mode, the 32-bit context is stored in the cpu area */
                if (!ReadProcessMemory( pi.hProcess, teb.TlsSlots[WOW64_TLS_CPURESERVED],
                                        cpu, cpu_size, &res )) res = 0;
                ok( res == cpu_size, "wrong len %lx\n", res );
                ok(ctx_ptr->ContextFlags == WOW64_CONTEXT_ALL,
                   "cs64: got context flags %#x\n", ctx_ptr->ContextFlags);
                ok(ctx_ptr->Eip == ctx.Eip, "cs64: got eip %08x / %08x\n", ctx_ptr->Eip, ctx.Eip);
                ok(ctx_ptr->Eax == ctx.Eax, "cs64: got eax %08x / %08x\n", ctx_ptr->Eax, ctx.Eax);
                ok(ctx_ptr->Ebx == ctx.Ebx, "cs64: got ebx %08x / %08x\n", ctx_ptr->Ebx, ctx.Ebx);
                ok(ctx_ptr->Ecx == ctx.Ecx, "cs64: got ecx %08x / %08x\n", ctx_ptr->Ecx, ctx.Ecx);
                ok(ctx_ptr->Edx == ctx.Edx, "cs64: got edx %08x / %08x\n", ctx_ptr->Edx, ctx.Edx);
                ok(ctx_ptr->Ebp == ctx.Ebp, "cs64: got ebp %08x / %08x\n", ctx_ptr->Ebp, ctx.Ebp);
                ok(ctx_ptr->Esi == ctx.Esi, "cs64: got esi %08x / %08x\n", ctx_ptr->Esi, ctx.Esi);
                ok(ctx_ptr->Edi == ctx.Edi, "cs64: got edi %08x / %08x\n", ctx_ptr->Edi, ctx.Edi);
                ok(ctx_ptr->EFlags == ctx.EFlags, "cs64: got eflags %08x / %08x\n", ctx_ptr->EFlags, ctx.EFlags);

                /* changing one context doesn't change the other one */
                rcx = context.Rcx;
                ecx = ctx.Ecx;
                context.Rcx = 0xfedcba987654321ull;
                pNtSetContextThread( pi.hThread, &context );
                memset( &ctx, 0x55, sizeof(ctx) );
                ctx.ContextFlags = WOW64_CONTEXT_ALL;
                pRtlWow64GetThreadContext( pi.hThread, &ctx );
                ok( ctx.Ecx == ecx, "cs64: ecx set to %08x\n", ctx.Ecx );
                ReadProcessMemory( pi.hProcess, teb.TlsSlots[WOW64_TLS_CPURESERVED], cpu, cpu_size, &res );
                ok( ctx_ptr->Ecx == ecx, "cs64: ecx set to %08x\n", ctx_ptr->Ecx );
                ctx.Ecx = 0x22223333;
                pRtlWow64SetThreadContext( pi.hThread, &ctx );
                memset( &ctx, 0x55, sizeof(ctx) );
                ctx.ContextFlags = WOW64_CONTEXT_ALL;
                pRtlWow64GetThreadContext( pi.hThread, &ctx );
                ok( ctx.Ecx == 0x22223333, "cs64: ecx set to %08x\n", ctx.Ecx );
                ReadProcessMemory( pi.hProcess, teb.TlsSlots[WOW64_TLS_CPURESERVED], cpu, cpu_size, &res );
                ok( ctx_ptr->Ecx == 0x22223333, "cs64: ecx set to %08x\n", ctx_ptr->Ecx );
                memset( &context, 0x55, sizeof(context) );
                context.ContextFlags = CONTEXT_ALL;
                pNtGetContextThread( pi.hThread, &context );
                ok( context.Rcx == 0xfedcba987654321ull, "cs64: rcx set to %p\n", (void *)context.Rcx );
                /* restore everything */
                context.Rcx = rcx;
                pNtSetContextThread( pi.hThread, &context );
                ctx.Ecx = ecx;
                pRtlWow64SetThreadContext( pi.hThread, &ctx );
            }
            got64 = TRUE;
        }
    }
    if (!got32) skip( "failed to test 32-bit context\n" );
    if (!got64) skip( "failed to test 64-bit context\n" );

done:
    pNtTerminateProcess(pi.hProcess, 0);
    free( cpu );
}

static BYTE saved_KiUserExceptionDispatcher_bytes[12];
static void *pKiUserExceptionDispatcher;
static BOOL hook_called;
static void *hook_KiUserExceptionDispatcher_rip;
static void *dbg_except_continue_handler_rip;
static void *hook_exception_address;
static struct
{
    ULONG64 old_rax;
    ULONG64 old_rdx;
    ULONG64 old_rsi;
    ULONG64 old_rdi;
    ULONG64 old_rbp;
    ULONG64 old_rsp;
    ULONG64 new_rax;
    ULONG64 new_rdx;
    ULONG64 new_rsi;
    ULONG64 new_rdi;
    ULONG64 new_rbp;
    ULONG64 new_rsp;
}
test_kiuserexceptiondispatcher_regs;

static ULONG64 test_kiuserexceptiondispatcher_saved_r12;

static DWORD dbg_except_continue_handler(EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
        CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher)
{
    trace("handler context->Rip %#lx, codemem %p.\n", context->Rip, code_mem);
    got_exception = 1;
    dbg_except_continue_handler_rip = (void *)context->Rip;
    ++context->Rip;
    memcpy(pKiUserExceptionDispatcher, saved_KiUserExceptionDispatcher_bytes,
            sizeof(saved_KiUserExceptionDispatcher_bytes));

    RtlUnwind((void *)test_kiuserexceptiondispatcher_regs.old_rsp,
            (BYTE *)code_mem + 0x28, rec, (void *)0xdeadbeef);
    return ExceptionContinueExecution;
}

static LONG WINAPI dbg_except_continue_vectored_handler(struct _EXCEPTION_POINTERS *e)
{
    EXCEPTION_RECORD *rec = e->ExceptionRecord;
    CONTEXT *context = e->ContextRecord;

    trace("dbg_except_continue_vectored_handler, code %#x, Rip %#lx.\n", rec->ExceptionCode, context->Rip);

    if (rec->ExceptionCode == 0xceadbeef)
    {
        ok(context->P1Home == (ULONG64)0xdeadbeeffeedcafe, "Got unexpected context->P1Home %#lx.\n", context->P1Home);
        context->R12 = test_kiuserexceptiondispatcher_saved_r12;
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    ok(rec->ExceptionCode == 0x80000003, "Got unexpected exception code %#x.\n", rec->ExceptionCode);

    got_exception = 1;
    dbg_except_continue_handler_rip = (void *)context->Rip;
    if (NtCurrentTeb()->Peb->BeingDebugged)
        ++context->Rip;

    if (context->Rip >= (ULONG64)code_mem && context->Rip < (ULONG64)code_mem + 0x100)
        RtlUnwind((void *)test_kiuserexceptiondispatcher_regs.old_rsp,
                (BYTE *)code_mem + 0x28, rec, (void *)0xdeadbeef);

    return EXCEPTION_CONTINUE_EXECUTION;
}

static void WINAPI hook_KiUserExceptionDispatcher(EXCEPTION_RECORD *rec, CONTEXT *context)
{
    trace("rec %p, context %p.\n", rec, context);
    trace("context->Rip %#lx, context->Rsp %#lx, ContextFlags %#lx.\n",
            context->Rip, context->Rsp, context->ContextFlags);

    hook_called = TRUE;
    /* Broken on Win2008, probably rec offset in stack is different. */
    ok(rec->ExceptionCode == 0x80000003 || rec->ExceptionCode == 0xceadbeef || broken(!rec->ExceptionCode),
            "Got unexpected ExceptionCode %#x.\n", rec->ExceptionCode);

    hook_KiUserExceptionDispatcher_rip = (void *)context->Rip;
    hook_exception_address = rec->ExceptionAddress;
    memcpy(pKiUserExceptionDispatcher, saved_KiUserExceptionDispatcher_bytes,
            sizeof(saved_KiUserExceptionDispatcher_bytes));
}

static void test_kiuserexceptiondispatcher(void)
{
    LPVOID vectored_handler;
    HMODULE hntdll = GetModuleHandleA("ntdll.dll");
    static BYTE except_code[] =
    {
        0x48, 0xb9, /* mov imm64, %rcx */
        /* offset: 0x2 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

        0x48, 0x89, 0x01,       /* mov %rax, (%rcx) */
        0x48, 0x89, 0x51, 0x08, /* mov %rdx, 0x8(%rcx) */
        0x48, 0x89, 0x71, 0x10, /* mov %rsi, 0x10(%rcx) */
        0x48, 0x89, 0x79, 0x18, /* mov %rdi, 0x18(%rcx) */
        0x48, 0x89, 0x69, 0x20, /* mov %rbp, 0x20(%rcx) */
        0x48, 0x89, 0x61, 0x28, /* mov %rsp, 0x28(%rcx) */
        0x48, 0x83, 0xc1, 0x30, /* add $0x30, %rcx */

        /* offset: 0x25 */
        0xcc,  /* int3 */

        0x0f, 0x0b, /* ud2, illegal instruction */

        /* offset: 0x28 */
        0x48, 0xb9, /* mov imm64, %rcx */
        /* offset: 0x2a */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

        0x48, 0x89, 0x01,       /* mov %rax, (%rcx) */
        0x48, 0x89, 0x51, 0x08, /* mov %rdx, 0x8(%rcx) */
        0x48, 0x89, 0x71, 0x10, /* mov %rsi, 0x10(%rcx) */
        0x48, 0x89, 0x79, 0x18, /* mov %rdi, 0x18(%rcx) */
        0x48, 0x89, 0x69, 0x20, /* mov %rbp, 0x20(%rcx) */
        0x48, 0x89, 0x61, 0x28, /* mov %rsp, 0x28(%rcx) */
        0xc3,  /* ret  */
    };
    static BYTE hook_trampoline[] =
    {
        0x48, 0x89, 0xe2,           /* mov %rsp,%rdx */
        0x48, 0x8d, 0x8c, 0x24, 0xf0, 0x04, 0x00, 0x00,
                                    /* lea 0x4f0(%rsp),%rcx */
        0x4c, 0x89, 0x22,           /* mov %r12,(%rdx) */
        0x48, 0xb8,                 /* movabs hook_KiUserExceptionDispatcher,%rax */
        0,0,0,0,0,0,0,0,            /* offset 16 */
        0xff, 0xd0,                 /* callq *rax */
        0x48, 0x31, 0xc9,           /* xor %rcx, %rcx */
        0x48, 0x31, 0xd2,           /* xor %rdx, %rdx */
        0x48, 0xb8,                 /* movabs pKiUserExceptionDispatcher,%rax */
        0,0,0,0,0,0,0,0,            /* offset 34 */
        0xff, 0xe0,                 /* jmpq *rax */
    };

    BYTE patched_KiUserExceptionDispatcher_bytes[12];
    void *bpt_address, *trampoline_ptr;
    EXCEPTION_RECORD record;
    DWORD old_protect;
    CONTEXT ctx;
    LONG pass;
    BYTE *ptr;
    BOOL ret;

    pKiUserExceptionDispatcher = (void *)GetProcAddress(hntdll, "KiUserExceptionDispatcher");
    if (!pKiUserExceptionDispatcher)
    {
        win_skip("KiUserExceptionDispatcher is not available.\n");
        return;
    }

    *(ULONG64 *)(except_code + 2) = (ULONG64)&test_kiuserexceptiondispatcher_regs;
    *(ULONG64 *)(except_code + 0x2a) = (ULONG64)&test_kiuserexceptiondispatcher_regs.new_rax;

    *(ULONG_PTR *)(hook_trampoline + 16) = (ULONG_PTR)hook_KiUserExceptionDispatcher;
    *(ULONG_PTR *)(hook_trampoline + 34) = (ULONG_PTR)pKiUserExceptionDispatcher;
    trampoline_ptr = (char *)code_mem + 1024;
    memcpy(trampoline_ptr, hook_trampoline, sizeof(hook_trampoline));
    ok(((ULONG64)trampoline_ptr & 0xffffffff) == (ULONG64)trampoline_ptr, "Address is too long.\n");

    ret = VirtualProtect(pKiUserExceptionDispatcher, sizeof(saved_KiUserExceptionDispatcher_bytes),
            PAGE_EXECUTE_READWRITE, &old_protect);
    ok(ret, "Got unexpected ret %#x, GetLastError() %u.\n", ret, GetLastError());

    memcpy(saved_KiUserExceptionDispatcher_bytes, pKiUserExceptionDispatcher,
            sizeof(saved_KiUserExceptionDispatcher_bytes));
    ptr = (BYTE *)patched_KiUserExceptionDispatcher_bytes;
    /* mov hook_trampoline, %rax */
    *ptr++ = 0x48;
    *ptr++ = 0xb8;
    *(void **)ptr = trampoline_ptr;
    ptr += sizeof(ULONG64);
    /* jmp *rax */
    *ptr++ = 0xff;
    *ptr++ = 0xe0;

    memcpy(pKiUserExceptionDispatcher, patched_KiUserExceptionDispatcher_bytes,
            sizeof(patched_KiUserExceptionDispatcher_bytes));
    got_exception = 0;
    run_exception_test(dbg_except_continue_handler, NULL, except_code, ARRAY_SIZE(except_code), PAGE_EXECUTE_READ);
    ok(got_exception, "Handler was not called.\n");
    ok(hook_called, "Hook was not called.\n");

    ok(test_kiuserexceptiondispatcher_regs.new_rax == 0xdeadbeef, "Got unexpected rax %#lx.\n",
            test_kiuserexceptiondispatcher_regs.new_rax);
    ok(test_kiuserexceptiondispatcher_regs.old_rsi
            == test_kiuserexceptiondispatcher_regs.new_rsi, "rsi does not match.\n");
    ok(test_kiuserexceptiondispatcher_regs.old_rdi
            == test_kiuserexceptiondispatcher_regs.new_rdi, "rdi does not match.\n");
    ok(test_kiuserexceptiondispatcher_regs.old_rbp
            == test_kiuserexceptiondispatcher_regs.new_rbp, "rbp does not match.\n");

    bpt_address = (BYTE *)code_mem + 0x25;

    ok(hook_exception_address == bpt_address || broken(!hook_exception_address) /* Win2008 */,
            "Got unexpected exception address %p, expected %p.\n",
            hook_exception_address, bpt_address);
    ok(hook_KiUserExceptionDispatcher_rip == bpt_address, "Got unexpected exception address %p, expected %p.\n",
            hook_KiUserExceptionDispatcher_rip, bpt_address);
    ok(dbg_except_continue_handler_rip == bpt_address, "Got unexpected exception address %p, expected %p.\n",
            dbg_except_continue_handler_rip, bpt_address);

    memset(&record, 0, sizeof(record));
    record.ExceptionCode = 0x80000003;
    record.ExceptionFlags = 0;
    record.ExceptionRecord = NULL;
    record.ExceptionAddress = NULL;
    record.NumberParameters = 0;

    vectored_handler = AddVectoredExceptionHandler(TRUE, dbg_except_continue_vectored_handler);

    memcpy(pKiUserExceptionDispatcher, patched_KiUserExceptionDispatcher_bytes,
            sizeof(patched_KiUserExceptionDispatcher_bytes));
    got_exception = 0;
    hook_called = FALSE;

    pRtlRaiseException(&record);

    ok(got_exception, "Handler was not called.\n");
    ok(!hook_called, "Hook was called.\n");

    memcpy(pKiUserExceptionDispatcher, patched_KiUserExceptionDispatcher_bytes,
            sizeof(patched_KiUserExceptionDispatcher_bytes));
    got_exception = 0;
    hook_called = FALSE;
    NtCurrentTeb()->Peb->BeingDebugged = 1;

    pRtlRaiseException(&record);

    ok(got_exception, "Handler was not called.\n");
    ok(hook_called, "Hook was not called.\n");

    ok(hook_exception_address == (BYTE *)hook_KiUserExceptionDispatcher_rip + 1
            || broken(!hook_exception_address) /* 2008 */, "Got unexpected addresses %p, %p.\n",
            hook_KiUserExceptionDispatcher_rip, hook_exception_address);

    RemoveVectoredExceptionHandler(vectored_handler);

    memcpy(pKiUserExceptionDispatcher, patched_KiUserExceptionDispatcher_bytes,
            sizeof(patched_KiUserExceptionDispatcher_bytes));
    got_exception = 0;
    hook_called = FALSE;

    run_exception_test(dbg_except_continue_handler, NULL, except_code, ARRAY_SIZE(except_code), PAGE_EXECUTE_READ);

    ok(got_exception, "Handler was not called.\n");
    ok(hook_called, "Hook was not called.\n");
    ok(hook_KiUserExceptionDispatcher_rip == bpt_address, "Got unexpected exception address %p, expected %p.\n",
            hook_KiUserExceptionDispatcher_rip, bpt_address);
    ok(dbg_except_continue_handler_rip == bpt_address, "Got unexpected exception address %p, expected %p.\n",
            dbg_except_continue_handler_rip, bpt_address);

    ok(test_kiuserexceptiondispatcher_regs.new_rax == 0xdeadbeef, "Got unexpected rax %#lx.\n",
            test_kiuserexceptiondispatcher_regs.new_rax);
    ok(test_kiuserexceptiondispatcher_regs.old_rsi
            == test_kiuserexceptiondispatcher_regs.new_rsi, "rsi does not match.\n");
    ok(test_kiuserexceptiondispatcher_regs.old_rdi
            == test_kiuserexceptiondispatcher_regs.new_rdi, "rdi does not match.\n");
    ok(test_kiuserexceptiondispatcher_regs.old_rbp
            == test_kiuserexceptiondispatcher_regs.new_rbp, "rbp does not match.\n");

    NtCurrentTeb()->Peb->BeingDebugged = 0;

    vectored_handler = AddVectoredExceptionHandler(TRUE, dbg_except_continue_vectored_handler);
    pass = 0;
    InterlockedIncrement(&pass);
    pRtlCaptureContext(&ctx);
    if (InterlockedIncrement(&pass) == 2) /* interlocked to prevent compiler from moving before capture */
    {
        memcpy(pKiUserExceptionDispatcher, patched_KiUserExceptionDispatcher_bytes,
                sizeof(patched_KiUserExceptionDispatcher_bytes));
        got_exception = 0;
        hook_called = FALSE;

        record.ExceptionCode = 0xceadbeef;
        test_kiuserexceptiondispatcher_saved_r12 = ctx.R12;
        ctx.R12 = (ULONG64)0xdeadbeeffeedcafe;

#ifdef __GNUC__
        /* Spoil r12 value to make sure it doesn't come from the current userspace registers. */
        __asm__ volatile("movq $0xdeadcafe, %%r12" : : : "%r12");
#endif
        pNtRaiseException(&record, &ctx, TRUE);
        ok(0, "Shouldn't be reached.\n");
    }
    else
    {
        ok(pass == 3, "Got unexpected pass %d.\n", pass);
    }
    ok(hook_called, "Hook was not called.\n");
    RemoveVectoredExceptionHandler(vectored_handler);

    ret = VirtualProtect(pKiUserExceptionDispatcher, sizeof(saved_KiUserExceptionDispatcher_bytes),
            old_protect, &old_protect);
    ok(ret, "Got unexpected ret %#x, GetLastError() %u.\n", ret, GetLastError());
}

static BOOL got_nested_exception, got_prev_frame_exception;
static void *nested_exception_initial_frame;

static DWORD nested_exception_handler(EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
        CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher)
{
    trace("nested_exception_handler Rip %p, Rsp %p, code %#x, flags %#x, ExceptionAddress %p.\n",
            (void *)context->Rip, (void *)context->Rsp, rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress);

    if (rec->ExceptionCode == 0x80000003
            && !(rec->ExceptionFlags & EH_NESTED_CALL))
    {
        ok(rec->NumberParameters == 1, "Got unexpected rec->NumberParameters %u.\n", rec->NumberParameters);
        ok((void *)context->Rsp == frame, "Got unexpected frame %p.\n", frame);
        ok(*(void **)frame == (char *)code_mem + 5, "Got unexpected *frame %p.\n", *(void **)frame);
        ok(context->Rip == (ULONG_PTR)((char *)code_mem + 7), "Got unexpected Rip %#lx.\n", context->Rip);

        nested_exception_initial_frame = frame;
        RaiseException(0xdeadbeef, 0, 0, 0);
        ++context->Rip;
        return ExceptionContinueExecution;
    }

    if (rec->ExceptionCode == 0xdeadbeef && rec->ExceptionFlags == EH_NESTED_CALL)
    {
        ok(!rec->NumberParameters, "Got unexpected rec->NumberParameters %u.\n", rec->NumberParameters);
        got_nested_exception = TRUE;
        ok(frame == nested_exception_initial_frame, "Got unexpected frame %p.\n", frame);
        return ExceptionContinueSearch;
    }

    ok(rec->ExceptionCode == 0xdeadbeef && !rec->ExceptionFlags,
            "Got unexpected exception code %#x, flags %#x.\n", rec->ExceptionCode, rec->ExceptionFlags);
    ok(!rec->NumberParameters, "Got unexpected rec->NumberParameters %u.\n", rec->NumberParameters);
    ok(frame == (void *)((BYTE *)nested_exception_initial_frame + 8),
            "Got unexpected frame %p.\n", frame);
    got_prev_frame_exception = TRUE;
    return ExceptionContinueExecution;
}

static void test_nested_exception(void)
{
    static const BYTE except_code[] =
    {
        0xe8, 0x02, 0x00, 0x00, 0x00, /* call nest */
        0x90,                         /* nop */
        0xc3,                         /* ret */
        /* nest: */
        0xcc,                         /* int3 */
        0x90,                         /* nop */
        0xc3,                         /* ret  */
    };

    got_nested_exception = got_prev_frame_exception = FALSE;
    run_exception_test(nested_exception_handler, NULL, except_code, ARRAY_SIZE(except_code), PAGE_EXECUTE_READ);
    ok(got_nested_exception, "Did not get nested exception.\n");
    ok(got_prev_frame_exception, "Did not get nested exception in the previous frame.\n");
}

static CONTEXT test_unwind_apc_context;
static BOOL test_unwind_apc_called;

static void CALLBACK test_unwind_apc(ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3)
{
    EXCEPTION_RECORD rec;

    test_unwind_apc_called = TRUE;
    memset(&rec, 0, sizeof(rec));
    pRtlUnwind((void *)test_unwind_apc_context.Rsp, (void *)test_unwind_apc_context.Rip, &rec, (void *)0xdeadbeef);
    ok(0, "Should not get here.\n");
}

static void test_unwind_from_apc(void)
{
    NTSTATUS status;
    int pass;

    if (!pNtQueueApcThread)
    {
        win_skip("NtQueueApcThread is not available.\n");
        return;
    }

    pass = 0;
    InterlockedIncrement(&pass);
    RtlCaptureContext(&test_unwind_apc_context);
    InterlockedIncrement(&pass);

    if (pass == 2)
    {
        test_unwind_apc_called = FALSE;
        status = pNtQueueApcThread(GetCurrentThread(), test_unwind_apc, 0, 0, 0);
        ok(!status, "Got unexpected status %#x.\n", status);
        SleepEx(0, TRUE);
        ok(0, "Should not get here.\n");
    }
    if (pass == 3)
    {
        ok(test_unwind_apc_called, "Test user APC was not called.\n");
        test_unwind_apc_called = FALSE;
        status = pNtQueueApcThread(GetCurrentThread(), test_unwind_apc, 0, 0, 0);
        ok(!status, "Got unexpected status %#x.\n", status);
        NtContinue(&test_unwind_apc_context, TRUE );
        ok(0, "Should not get here.\n");
    }
    ok(pass == 4, "Got unexpected pass %d.\n", pass);
    ok(test_unwind_apc_called, "Test user APC was not called.\n");
}
#elif defined(__arm__)

static void test_thread_context(void)
{
    CONTEXT context;
    NTSTATUS status;
    struct expected
    {
        DWORD R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, Sp, Lr, Pc, Cpsr;
    } expect;
    NTSTATUS (*func_ptr)( void *arg1, void *arg2, struct expected *res, void *func );

    static const WORD call_func[] =
    {
        0xb502,            /* push    {r1, lr} */
        0xe882, 0x1fff,    /* stmia.w r2, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip} */
        0xf8c2, 0xd034,    /* str.w   sp, [r2, #52] */
        0xf8c2, 0xe038,    /* str.w   lr, [r2, #56] */
        0xf3ef, 0x8100,    /* mrs     r1, CPSR */
        0xf041, 0x0120,    /* orr.w   r1, r1, #32 */
        0x6411,            /* str     r1, [r2, #64] */
        0x9900,            /* ldr     r1, [sp, #0] */
        0x4679,            /* mov     r1, pc */
        0xf101, 0x0109,    /* add.w   r1, r1, #9 */
        0x63d1,            /* str     r1, [r2, #60] */
        0x9900,            /* ldr     r1, [sp, #0] */
        0x4798,            /* blx     r3 */
        0xbd02,            /* pop     {r1, pc} */
    };

    memcpy( code_mem, call_func, sizeof(call_func) );
    func_ptr = (void *)((char *)code_mem + 1);  /* thumb */

#define COMPARE(reg) \
    ok( context.reg == expect.reg, "wrong " #reg " %08x/%08x\n", context.reg, expect.reg )

    memset( &context, 0xcc, sizeof(context) );
    memset( &expect, 0xcc, sizeof(expect) );
    func_ptr( &context, 0, &expect, pRtlCaptureContext );
    trace( "expect: r0=%08x r1=%08x r2=%08x r3=%08x r4=%08x r5=%08x r6=%08x r7=%08x r8=%08x r9=%08x "
           "r10=%08x r11=%08x r12=%08x sp=%08x lr=%08x pc=%08x cpsr=%08x\n",
           expect.R0, expect.R1, expect.R2, expect.R3, expect.R4, expect.R5, expect.R6, expect.R7,
           expect.R8, expect.R9, expect.R10, expect.R11, expect.R12, expect.Sp, expect.Lr, expect.Pc, expect.Cpsr );
    trace( "actual: r0=%08x r1=%08x r2=%08x r3=%08x r4=%08x r5=%08x r6=%08x r7=%08x r8=%08x r9=%08x "
           "r10=%08x r11=%08x r12=%08x sp=%08x lr=%08x pc=%08x cpsr=%08x\n",
           context.R0, context.R1, context.R2, context.R3, context.R4, context.R5, context.R6, context.R7,
           context.R8, context.R9, context.R10, context.R11, context.R12, context.Sp, context.Lr, context.Pc, context.Cpsr );

    ok( context.ContextFlags == (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_FLOATING_POINT),
        "wrong flags %08x\n", context.ContextFlags );
    ok( !context.R0, "wrong R0 %08x\n", context.R0 );
    COMPARE( R1 );
    COMPARE( R2 );
    COMPARE( R3 );
    COMPARE( R4 );
    COMPARE( R5 );
    COMPARE( R6 );
    COMPARE( R7 );
    COMPARE( R8 );
    COMPARE( R9 );
    COMPARE( R10 );
    COMPARE( R11 );
    COMPARE( R12 );
    COMPARE( Sp );
    COMPARE( Pc );
    COMPARE( Cpsr );
    ok( !context.Lr, "wrong Lr %08x\n", context.Lr );

    memset( &context, 0xcc, sizeof(context) );
    memset( &expect, 0xcc, sizeof(expect) );
    context.ContextFlags = CONTEXT_FULL;

    status = func_ptr( GetCurrentThread(), &context, &expect, pNtGetContextThread );
    ok( status == STATUS_SUCCESS, "NtGetContextThread failed %08x\n", status );
    trace( "expect: r0=%08x r1=%08x r2=%08x r3=%08x r4=%08x r5=%08x r6=%08x r7=%08x r8=%08x r9=%08x "
           "r10=%08x r11=%08x r12=%08x sp=%08x lr=%08x pc=%08x cpsr=%08x\n",
           expect.R0, expect.R1, expect.R2, expect.R3, expect.R4, expect.R5, expect.R6, expect.R7,
           expect.R8, expect.R9, expect.R10, expect.R11, expect.R12, expect.Sp, expect.Lr, expect.Pc, expect.Cpsr );
    trace( "actual: r0=%08x r1=%08x r2=%08x r3=%08x r4=%08x r5=%08x r6=%08x r7=%08x r8=%08x r9=%08x "
           "r10=%08x r11=%08x r12=%08x sp=%08x lr=%08x pc=%08x cpsr=%08x\n",
           context.R0, context.R1, context.R2, context.R3, context.R4, context.R5, context.R6, context.R7,
           context.R8, context.R9, context.R10, context.R11, context.R12, context.Sp, context.Lr, context.Pc, context.Cpsr );
    /* other registers are not preserved */
    COMPARE( R4 );
    COMPARE( R5 );
    COMPARE( R6 );
    COMPARE( R7 );
    COMPARE( R8 );
    COMPARE( R9 );
    COMPARE( R10 );
    COMPARE( R11 );
    ok( (context.Cpsr & 0xff0f0030) == (expect.Cpsr & 0xff0f0030),
        "wrong Cpsr %08x/%08x\n", context.Cpsr, expect.Cpsr );
    ok( context.Sp == expect.Sp - 16,
        "wrong Sp %08x/%08x\n", context.Sp, expect.Sp - 16 );
    /* Pc is somewhere close to the NtGetContextThread implementation */
    ok( (char *)context.Pc >= (char *)pNtGetContextThread &&
        (char *)context.Pc <= (char *)pNtGetContextThread + 0x10,
        "wrong Pc %08x/%08x\n", context.Pc, (DWORD)pNtGetContextThread );
#undef COMPARE
}

static void test_debugger(DWORD cont_status)
{
    char cmdline[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { 0 };
    DEBUG_EVENT de;
    DWORD continuestatus;
    PVOID code_mem_address = NULL;
    NTSTATUS status;
    SIZE_T size_read;
    BOOL ret;
    int counter = 0;
    si.cb = sizeof(si);

    if(!pNtGetContextThread || !pNtSetContextThread || !pNtReadVirtualMemory || !pNtTerminateProcess)
    {
        skip("NtGetContextThread, NtSetContextThread, NtReadVirtualMemory or NtTerminateProcess not found\n");
        return;
    }

    sprintf(cmdline, "%s %s %s %p", my_argv[0], my_argv[1], "debuggee", &test_stage);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, NULL, &si, &pi);
    ok(ret, "could not create child process error: %u\n", GetLastError());
    if (!ret)
        return;

    do
    {
        continuestatus = cont_status;
        ok(WaitForDebugEvent(&de, INFINITE), "reading debug event\n");

        ret = ContinueDebugEvent(de.dwProcessId, de.dwThreadId, 0xdeadbeef);
        ok(!ret, "ContinueDebugEvent unexpectedly succeeded\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "Unexpected last error: %u\n", GetLastError());

        if (de.dwThreadId != pi.dwThreadId)
        {
            trace("event %d not coming from main thread, ignoring\n", de.dwDebugEventCode);
            ContinueDebugEvent(de.dwProcessId, de.dwThreadId, cont_status);
            continue;
        }

        if (de.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
        {
            if(de.u.CreateProcessInfo.lpBaseOfImage != NtCurrentTeb()->Peb->ImageBaseAddress)
            {
                skip("child process loaded at different address, terminating it\n");
                pNtTerminateProcess(pi.hProcess, 0);
            }
        }
        else if (de.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
        {
            CONTEXT ctx;
            int stage;

            counter++;
            status = pNtReadVirtualMemory(pi.hProcess, &code_mem, &code_mem_address,
                                          sizeof(code_mem_address), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);
            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);

            ctx.ContextFlags = CONTEXT_FULL;
            status = pNtGetContextThread(pi.hThread, &ctx);
            ok(!status, "NtGetContextThread failed with 0x%x\n", status);

            trace("exception 0x%x at %p firstchance=%d pc=%08x, r0=%08x\n",
                  de.u.Exception.ExceptionRecord.ExceptionCode,
                  de.u.Exception.ExceptionRecord.ExceptionAddress,
                  de.u.Exception.dwFirstChance, ctx.Pc, ctx.R0);

            if (counter > 100)
            {
                ok(FALSE, "got way too many exceptions, probably caught in an infinite loop, terminating child\n");
                pNtTerminateProcess(pi.hProcess, 1);
            }
            else if (counter < 2) /* startup breakpoint */
            {
                /* breakpoint is inside ntdll */
                void *ntdll = GetModuleHandleA( "ntdll.dll" );
                IMAGE_NT_HEADERS *nt = RtlImageNtHeader( ntdll );

                ok( (char *)ctx.Pc >= (char *)ntdll &&
                    (char *)ctx.Pc < (char *)ntdll + nt->OptionalHeader.SizeOfImage,
                    "wrong pc %p ntdll %p-%p\n", (void *)ctx.Pc, ntdll,
                    (char *)ntdll + nt->OptionalHeader.SizeOfImage );
            }
            else
            {
                if (stage == 1)
                {
                    ok((char *)ctx.Pc == (char *)code_mem_address + 0xb, "Pc at %x instead of %p\n",
                       ctx.Pc, (char *)code_mem_address + 0xb);
                    /* setting the context from debugger does not affect the context that the
                     * exception handler gets, except on w2008 */
                    ctx.Pc = (UINT_PTR)code_mem_address + 0xd;
                    ctx.R0 = 0xf00f00f1;
                    /* let the debuggee handle the exception */
                    continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == 2)
                {
                    if (de.u.Exception.dwFirstChance)
                    {
                        /* debugger gets first chance exception with unmodified ctx.Pc */
                        ok((char *)ctx.Pc == (char *)code_mem_address + 0xb, "Pc at 0x%x instead of %p\n",
                           ctx.Pc, (char *)code_mem_address + 0xb);
                        ctx.Pc = (UINT_PTR)code_mem_address + 0xd;
                        ctx.R0 = 0xf00f00f1;
                        /* pass exception to debuggee
                         * exception will not be handled and a second chance exception will be raised */
                        continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                    }
                    else
                    {
                        /* debugger gets context after exception handler has played with it */
                        /* ctx.Pc is the same value the exception handler got */
                        if (de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
                        {
                            ok((char *)ctx.Pc == (char *)code_mem_address + 0xa,
                               "Pc at 0x%x instead of %p\n", ctx.Pc, (char *)code_mem_address + 0xa);
                            /* need to fixup Pc for debuggee */
                            ctx.Pc += 2;
                        }
                        else ok((char *)ctx.Pc == (char *)code_mem_address + 0xb,
                                "Pc at 0x%x instead of %p\n", ctx.Pc, (char *)code_mem_address + 0xb);
                        /* here we handle exception */
                    }
                }
                else if (stage == 7 || stage == 8)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT,
                       "expected EXCEPTION_BREAKPOINT, got %08x\n", de.u.Exception.ExceptionRecord.ExceptionCode);
                    ok((char *)ctx.Pc == (char *)code_mem_address + 0x1d,
                       "expected Pc = %p, got 0x%x\n", (char *)code_mem_address + 0x1d, ctx.Pc);
                    if (stage == 8) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == 9 || stage == 10)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT,
                       "expected EXCEPTION_BREAKPOINT, got %08x\n", de.u.Exception.ExceptionRecord.ExceptionCode);
                    ok((char *)ctx.Pc == (char *)code_mem_address + 3,
                       "expected Pc = %p, got 0x%x\n", (char *)code_mem_address + 3, ctx.Pc);
                    if (stage == 10) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == 11 || stage == 12 || stage == 13)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_INVALID_HANDLE,
                       "unexpected exception code %08x, expected %08x\n", de.u.Exception.ExceptionRecord.ExceptionCode,
                       EXCEPTION_INVALID_HANDLE);
                    ok(de.u.Exception.ExceptionRecord.NumberParameters == 0,
                       "unexpected number of parameters %d, expected 0\n", de.u.Exception.ExceptionRecord.NumberParameters);

                    if (stage == 12|| stage == 13) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else
                    ok(FALSE, "unexpected stage %x\n", stage);

                status = pNtSetContextThread(pi.hThread, &ctx);
                ok(!status, "NtSetContextThread failed with 0x%x\n", status);
            }
        }
        else if (de.dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT)
        {
            int stage;
            char buffer[64];

            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);

            ok(!de.u.DebugString.fUnicode, "unexpected unicode debug string event\n");
            ok(de.u.DebugString.nDebugStringLength < sizeof(buffer) - 1, "buffer not large enough to hold %d bytes\n",
               de.u.DebugString.nDebugStringLength);

            memset(buffer, 0, sizeof(buffer));
            status = pNtReadVirtualMemory(pi.hProcess, de.u.DebugString.lpDebugStringData, buffer,
                                          de.u.DebugString.nDebugStringLength, &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);

            if (stage == 3 || stage == 4)
                ok(!strcmp(buffer, "Hello World"), "got unexpected debug string '%s'\n", buffer);
            else /* ignore unrelated debug strings like 'SHIMVIEW: ShimInfo(Complete)' */
                ok(strstr(buffer, "SHIMVIEW") != NULL, "unexpected stage %x, got debug string event '%s'\n", stage, buffer);

            if (stage == 4) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
        }
        else if (de.dwDebugEventCode == RIP_EVENT)
        {
            int stage;

            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);

            if (stage == 5 || stage == 6)
            {
                ok(de.u.RipInfo.dwError == 0x11223344, "got unexpected rip error code %08x, expected %08x\n",
                   de.u.RipInfo.dwError, 0x11223344);
                ok(de.u.RipInfo.dwType  == 0x55667788, "got unexpected rip type %08x, expected %08x\n",
                   de.u.RipInfo.dwType, 0x55667788);
            }
            else
                ok(FALSE, "unexpected stage %x\n", stage);

            if (stage == 6) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
        }

        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, continuestatus);

    } while (de.dwDebugEventCode != EXIT_PROCESS_DEBUG_EVENT);

    wait_child_process( pi.hProcess );
    ret = CloseHandle(pi.hThread);
    ok(ret, "error %u\n", GetLastError());
    ret = CloseHandle(pi.hProcess);
    ok(ret, "error %u\n", GetLastError());
}

static void test_debug_service(DWORD numexc)
{
    /* not supported */
}

#elif defined(__aarch64__)

#define UNW_FLAG_NHANDLER  0
#define UNW_FLAG_EHANDLER  1
#define UNW_FLAG_UHANDLER  2

#define UWOP_TWOBYTES(x) (((x) >> 8) & 0xff), ((x) & 0xff)

#define UWOP_ALLOC_SMALL(size)         (0x00 | (size/16))
#define UWOP_SAVE_R19R20_X(offset)     (0x20 | (offset/8))
#define UWOP_SAVE_FPLR(offset)         (0x40 | (offset/8))
#define UWOP_SAVE_FPLR_X(offset)       (0x80 | (offset/8 - 1))
#define UWOP_ALLOC_MEDIUM(size)        UWOP_TWOBYTES((0xC0 << 8) | (size/16))
#define UWOP_SAVE_REGP(reg, offset)    UWOP_TWOBYTES((0xC8 << 8) | ((reg - 19) << 6) | (offset/8))
#define UWOP_SAVE_REGP_X(reg, offset)  UWOP_TWOBYTES((0xCC << 8) | ((reg - 19) << 6) | (offset/8 - 1))
#define UWOP_SAVE_REG(reg, offset)     UWOP_TWOBYTES((0xD0 << 8) | ((reg - 19) << 6) | (offset/8))
#define UWOP_SAVE_REG_X(reg, offset)   UWOP_TWOBYTES((0xD4 << 8) | ((reg - 19) << 5) | (offset/8 - 1))
#define UWOP_SAVE_LRP(reg, offset)     UWOP_TWOBYTES((0xD6 << 8) | ((reg - 19)/2 << 6) | (offset/8))
#define UWOP_SAVE_FREGP(reg, offset)   UWOP_TWOBYTES((0xD8 << 8) | ((reg - 8) << 6) | (offset/8))
#define UWOP_SAVE_FREGP_X(reg, offset) UWOP_TWOBYTES((0xDA << 8) | ((reg - 8) << 6) | (offset/8 - 1))
#define UWOP_SAVE_FREG(reg, offset)    UWOP_TWOBYTES((0xDC << 8) | ((reg - 8) << 6) | (offset/8))
#define UWOP_SAVE_FREG_X(reg, offset)  UWOP_TWOBYTES((0xDE << 8) | ((reg - 8) << 5) | (offset/8 - 1))
#define UWOP_ALLOC_LARGE(size)         UWOP_TWOBYTES((0xE0 << 8) | ((size/16) >> 16)), UWOP_TWOBYTES(size/16)
#define UWOP_SET_FP                    0xE1
#define UWOP_ADD_FP(offset)            UWOP_TWOBYTES((0xE2 << 8) | (offset/8))
#define UWOP_NOP                       0xE3
#define UWOP_END                       0xE4
#define UWOP_END_C                     0xE5
#define UWOP_SAVE_NEXT                 0xE6
#define UWOP_TRAP_FRAME                0xE8
#define UWOP_MACHINE_FRAME             0xE9
#define UWOP_CONTEXT                   0xEA
#define UWOP_CLEAR_UNWOUND_TO_CALL     0xEC

struct results
{
    int pc_offset;      /* pc offset from code start */
    int fp_offset;      /* fp offset from stack pointer */
    int handler;        /* expect handler to be set? */
    ULONG_PTR pc;       /* expected final pc value */
    int frame;          /* expected frame return value */
    int frame_offset;   /* whether the frame return value is an offset or an absolute value */
    int regs[48][2];    /* expected values for registers */
};

struct unwind_test
{
    const BYTE *function;
    size_t function_size;
    const BYTE *unwind_info;
    size_t unwind_size;
    const struct results *results;
    unsigned int nb_results;
};

enum regs
{
    x0,  x1,  x2,  x3,  x4,  x5,  x6,  x7,
    x8,  x9,  x10, x11, x12, x13, x14, x15,
    x16, x17, x18, x19, x20, x21, x22, x23,
    x24, x25, x26, x27, x28, x29, lr,  sp,
    d0,  d1,  d2,  d3,  d4,  d5,  d6,  d7,
    d8,  d9,  d10, d11, d12, d13, d14, d15
};

static const char * const reg_names[48] =
{
    "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",
    "x8",  "x9",  "x10", "x11", "x12", "x13", "x14", "x15",
    "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
    "x24", "x25", "x26", "x27", "x28", "x29", "lr",  "sp",
    "d0",  "d1",  "d2",  "d3",  "d4",  "d5",  "d6",  "d7",
    "d8",  "d9",  "d10", "d11", "d12", "d13", "d14", "d15",
};

#define ORIG_LR 0xCCCCCCCC

#define UWOP(code,info) (UWOP_##code | ((info) << 4))

static void call_virtual_unwind( int testnum, const struct unwind_test *test )
{
    static const int code_offset = 1024;
    static const int unwind_offset = 2048;
    void *handler, *data;
    CONTEXT context;
    RUNTIME_FUNCTION runtime_func;
    KNONVOLATILE_CONTEXT_POINTERS ctx_ptr;
    UINT i, j, k;
    ULONG64 fake_stack[256];
    ULONG64 frame, orig_pc, orig_fp, unset_reg, sp_offset = 0;
    static const UINT nb_regs = ARRAY_SIZE(test->results[i].regs);

    memcpy( (char *)code_mem + code_offset, test->function, test->function_size );
    memcpy( (char *)code_mem + unwind_offset, test->unwind_info, test->unwind_size );

    runtime_func.BeginAddress = code_offset;
    if (test->unwind_size)
        runtime_func.UnwindData = unwind_offset;
    else
        memcpy(&runtime_func.UnwindData, test->unwind_info, 4);

    trace( "code: %p stack: %p\n", code_mem, fake_stack );

    for (i = 0; i < test->nb_results; i++)
    {
        memset( &ctx_ptr, 0, sizeof(ctx_ptr) );
        memset( &context, 0x55, sizeof(context) );
        memset( &unset_reg, 0x55, sizeof(unset_reg) );
        for (j = 0; j < 256; j++) fake_stack[j] = j * 8;

        context.Sp = (ULONG_PTR)fake_stack;
        context.Lr = (ULONG_PTR)ORIG_LR;
        context.Fp = (ULONG_PTR)fake_stack + test->results[i].fp_offset;
        orig_fp = context.Fp;
        orig_pc = (ULONG64)code_mem + code_offset + test->results[i].pc_offset;

        trace( "%u/%u: pc=%p (%02x) fp=%p sp=%p\n", testnum, i,
               (void *)orig_pc, *(DWORD *)orig_pc, (void *)orig_fp, (void *)context.Sp );

        data = (void *)0xdeadbeef;
        handler = RtlVirtualUnwind( UNW_FLAG_EHANDLER, (ULONG64)code_mem, orig_pc,
                                    &runtime_func, &context, &data, &frame, &ctx_ptr );
        if (test->results[i].handler > 0)
        {
            /* Yet untested */
            ok( (char *)handler == (char *)code_mem + 0x200,
                "%u/%u: wrong handler %p/%p\n", testnum, i, handler, (char *)code_mem + 0x200 );
            if (handler) ok( *(DWORD *)data == 0x08070605,
                             "%u/%u: wrong handler data %p\n", testnum, i, data );
        }
        else
        {
            ok( handler == NULL, "%u/%u: handler %p instead of NULL\n", testnum, i, handler );
            ok( data == (test->results[i].handler < 0 ?
                        (void *)0xdeadbeef : NULL),
                "%u/%u: handler data set to %p/%p\n", testnum, i, data,
                (test->results[i].handler < 0 ? (void *)0xdeadbeef : NULL) );
        }

        ok( context.Pc == test->results[i].pc, "%u/%u: wrong pc %p/%p\n",
            testnum, i, (void *)context.Pc, (void*)test->results[i].pc );
        ok( frame == (test->results[i].frame_offset ? (ULONG64)fake_stack : 0) + test->results[i].frame, "%u/%u: wrong frame %p/%p\n",
            testnum, i, (void *)frame, (char *)(test->results[i].frame_offset ? fake_stack : NULL) + test->results[i].frame );

        sp_offset = 0;
        for (k = 0; k < nb_regs; k++)
        {
            if (test->results[i].regs[k][0] == -1)
                break;
            if (test->results[i].regs[k][0] == sp) {
                /* If sp is part of the registers list, treat it as an offset
                 * between the returned frame pointer and the sp register. */
                sp_offset = test->results[i].regs[k][1];
                break;
            }
        }
        ok( frame - sp_offset == context.Sp, "%u/%u: wrong sp %p/%p\n",
            testnum, i, (void *)(frame - sp_offset), (void *)context.Sp);

        for (j = 0; j < 48; j++)
        {
            if (j == sp) continue; /* Handling sp separately above */

            for (k = 0; k < nb_regs; k++)
            {
                if (test->results[i].regs[k][0] == -1)
                {
                    k = nb_regs;
                    break;
                }
                if (test->results[i].regs[k][0] == j) break;
            }

            if (j >= 19 && j <= 30 && (&ctx_ptr.X19)[j - 19])
            {
                ok( k < nb_regs, "%u/%u: register %s should not be set to %llx\n",
                    testnum, i, reg_names[j], context.X[j] );
                if (k < nb_regs)
                    ok( context.X[j] == test->results[i].regs[k][1],
                        "%u/%u: register %s wrong %p/%x\n",
                        testnum, i, reg_names[j], (void *)context.X[j], test->results[i].regs[k][1] );
            }
            else if (j >= d8 && j <= d15 && (&ctx_ptr.D8)[j - d8])
            {
                ok( k < nb_regs, "%u/%u: register %s should not be set to %llx\n",
                    testnum, i, reg_names[j], context.V[j - d0].Low );
                if (k < nb_regs)
                    ok( context.V[j - d0].Low == test->results[i].regs[k][1],
                        "%u/%u: register %s wrong %p/%x\n",
                        testnum, i, reg_names[j], (void *)context.V[j - d0].Low, test->results[i].regs[k][1] );
            }
            else if (k < nb_regs)
            {
                if (j < d0)
                  ok( context.X[j] == test->results[i].regs[k][1],
                      "%u/%u: register %s wrong %p/%x\n",
                      testnum, i, reg_names[j], (void *)context.X[j], test->results[i].regs[k][1] );
                else
                  ok( context.V[j - d0].Low == test->results[i].regs[k][1],
                      "%u/%u: register %s wrong %p/%x\n",
                      testnum, i, reg_names[j], (void *)context.V[j - d0].Low, test->results[i].regs[k][1] );
            }
            else
            {
                ok( k == nb_regs, "%u/%u: register %s should be set\n", testnum, i, reg_names[j] );
                if (j == lr)
                    ok( context.Lr == ORIG_LR, "%u/%u: register lr wrong %p/unset\n",
                        testnum, i, (void *)context.Lr );
                else if (j == x29)
                    ok( context.Fp == orig_fp, "%u/%u: register fp wrong %p/unset\n",
                        testnum, i, (void *)context.Fp );
                else if (j < d0)
                    ok( context.X[j] == unset_reg,
                        "%u/%u: register %s wrong %p/unset\n",
                        testnum, i, reg_names[j], (void *)context.X[j]);
                else
                    ok( context.V[j - d0].Low == unset_reg,
                        "%u/%u: register %s wrong %p/unset\n",
                        testnum, i, reg_names[j], (void *)context.V[j - d0].Low);
            }
        }
    }
}

#define DW(dword) ((dword >> 0) & 0xff), ((dword >> 8) & 0xff), ((dword >> 16) & 0xff), ((dword >> 24) & 0xff)

static void test_virtual_unwind(void)
{
    static const BYTE function_0[] =
    {
        0xff, 0x83, 0x00, 0xd1,   /* 00: sub sp, sp, #32 */
        0xf3, 0x53, 0x01, 0xa9,   /* 04: stp x19, x20, [sp, #16] */
        0x1f, 0x20, 0x03, 0xd5,   /* 08: nop */
        0xf3, 0x53, 0x41, 0xa9,   /* 0c: ldp x19, x20, [sp, #16] */
        0xff, 0x83, 0x00, 0x91,   /* 10: add sp, sp, #32 */
        0xc0, 0x03, 0x5f, 0xd6,   /* 14: ret */
    };

    static const DWORD unwind_info_0_header =
        (sizeof(function_0)/4) | /* function length */
        (0 << 20) | /* X */
        (0 << 21) | /* E */
        (1 << 22) | /* epilog */
        (2 << 27);  /* codes */
    static const DWORD unwind_info_0_epilog0 =
        (3 <<  0) | /* offset */
        (4 << 22);  /* index */

    static const BYTE unwind_info_0[] =
    {
        DW(unwind_info_0_header),
        DW(unwind_info_0_epilog0),

        UWOP_SAVE_REGP(19, 16), /* stp x19, x20, [sp, #16] */
        UWOP_ALLOC_SMALL(32),   /* sub sp,  sp,  #32 */
        UWOP_END,

        UWOP_SAVE_REGP(19, 16), /* stp x19, x20, [sp, #16] */
        UWOP_ALLOC_SMALL(32),   /* sub sp,  sp,  #32 */
        UWOP_END,
    };

    static const struct results results_0[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     ORIG_LR, 0x020, TRUE, { {-1,-1} }},
        { 0x08,  0x00,  0,     ORIG_LR, 0x020, TRUE, { {x19,0x10}, {x20,0x18}, {-1,-1} }},
        { 0x0c,  0x00,  0,     ORIG_LR, 0x020, TRUE, { {x19,0x10}, {x20,0x18}, {-1,-1} }},
        { 0x10,  0x00,  0,     ORIG_LR, 0x020, TRUE, { {-1,-1} }},
        { 0x14,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };


    static const BYTE function_1[] =
    {
        0xf3, 0x53, 0xbe, 0xa9,   /* 00: stp x19, x20, [sp, #-32]! */
        0xfe, 0x0b, 0x00, 0xf9,   /* 04: str x30, [sp, #16] */
        0xff, 0x43, 0x00, 0xd1,   /* 08: sub sp, sp, #16 */
        0x1f, 0x20, 0x03, 0xd5,   /* 0c: nop */
        0xff, 0x43, 0x00, 0x91,   /* 10: add sp, sp, #16 */
        0xfe, 0x0b, 0x40, 0xf9,   /* 14: ldr x30, [sp, #16] */
        0xf3, 0x53, 0xc2, 0xa8,   /* 18: ldp x19, x20, [sp], #32 */
        0xc0, 0x03, 0x5f, 0xd6,   /* 1c: ret */
    };

    static const DWORD unwind_info_1_packed =
        (1 << 0)  | /* Flag */
        (sizeof(function_1)/4 << 2) | /* FunctionLength */
        (0 << 13) | /* RegF */
        (2 << 16) | /* RegI */
        (0 << 20) | /* H */
        (1 << 21) | /* CR */
        (3 << 23);  /* FrameSize */

    static const BYTE unwind_info_1[] = { DW(unwind_info_1_packed) };

    static const struct results results_1[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     ORIG_LR, 0x020, TRUE, { {x19,0x00}, {x20,0x08}, {-1,-1} }},
        { 0x08,  0x00,  0,     0x10,    0x020, TRUE, { {x19,0x00}, {x20,0x08}, {lr,0x10}, {-1,-1} }},
        { 0x0c,  0x00,  0,     0x20,    0x030, TRUE, { {x19,0x10}, {x20,0x18}, {lr,0x20}, {-1,-1} }},
        { 0x10,  0x00,  0,     0x20,    0x030, TRUE, { {x19,0x10}, {x20,0x18}, {lr,0x20}, {-1,-1} }},
        { 0x14,  0x00,  0,     0x10,    0x020, TRUE, { {x19,0x00}, {x20,0x08}, {lr,0x10}, {-1,-1} }},
        { 0x18,  0x00,  0,     ORIG_LR, 0x020, TRUE, { {x19,0x00}, {x20,0x08}, {-1,-1} }},
        { 0x1c,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_2[] =
    {
        0xff, 0x43, 0x00, 0xd1,   /* 00: sub sp, sp, #16 */
        0x1f, 0x20, 0x03, 0xd5,   /* 04: nop */
        0xff, 0x43, 0x00, 0xd1,   /* 08: sub sp, sp, #16 */
        0x1f, 0x20, 0x03, 0xd5,   /* 0c: nop */
        0xc0, 0x03, 0x5f, 0xd6,   /* 10: ret */
    };

    static const DWORD unwind_info_2_header =
        (sizeof(function_2)/4) | /* function length */
        (0 << 20) | /* X */
        (0 << 21) | /* E */
        (0 << 22) | /* epilog */
        (1 << 27);  /* codes */

    static const BYTE unwind_info_2[] =
    {
        DW(unwind_info_2_header),

        UWOP_ALLOC_SMALL(16),   /* sub sp,  sp,  #16 */
        UWOP_MACHINE_FRAME,
        UWOP_ALLOC_SMALL(16),   /* sub sp,  sp,  #16 */
        UWOP_END,
    };

    /* Partial prologues with the custom frame opcodes (machine frame,
     * context) behave like there's an off-by-one bug; unwinding from
     * offset 0, which normally does nothing, executes one opcode if
     * there's a machine frame or context in the prologue, and for other
     * offsets, it behaves like unwinding from one instruction further
     * ahead. So only test the full prologue case. */
    static const struct results results_2[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
#if 0
        { 0x00,  0x00,  0,     ORIG_LR, 0x010, TRUE,  { {-1,-1} }},
        { 0x04,  0x00,  0,     0x0008,  0x010, FALSE, { {-1,-1} }},
        { 0x08,  0x00,  0,     0x0018,  0x020, FALSE, { {-1,-1} }},
#endif
        { 0x0c,  0x00,  0,     0x0018,  0x020, FALSE, { {-1,-1} }},
    };

    static const BYTE function_3[] =
    {
        0xff, 0x43, 0x00, 0xd1,   /* 00: sub sp, sp, #16 */
        0x1f, 0x20, 0x03, 0xd5,   /* 04: nop */
        0xff, 0x43, 0x00, 0xd1,   /* 08: sub sp, sp, #16 */
        0x1f, 0x20, 0x03, 0xd5,   /* 0c: nop */
        0xc0, 0x03, 0x5f, 0xd6,   /* 10: ret */
    };

    static const DWORD unwind_info_3_header =
        (sizeof(function_3)/4) | /* function length */
        (0 << 20) | /* X */
        (0 << 21) | /* E */
        (0 << 22) | /* epilog */
        (1 << 27);  /* codes */

    static const BYTE unwind_info_3[] =
    {
        DW(unwind_info_3_header),

        UWOP_ALLOC_SMALL(16),   /* sub sp,  sp,  #16 */
        UWOP_CONTEXT,
        UWOP_ALLOC_SMALL(16),   /* sub sp,  sp,  #16 */
        UWOP_END,
    };

    static const struct results results_3[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
#if 0
        { 0x00,  0x00,  0,     ORIG_LR, 0x010, TRUE,  { {-1,-1} }},
        { 0x04,  0x00,  0 ,    0x0108,  0x110, FALSE, { {x0, 0x08}, {x1, 0x10}, {x2, 0x18}, {x3, 0x20}, {x4, 0x28}, {x5, 0x30}, {x6, 0x38}, {x7, 0x40}, {x8, 0x48}, {x9, 0x50}, {x10, 0x58}, {x11, 0x60}, {x12, 0x68}, {x13, 0x70}, {x14, 0x78}, {x15, 0x80}, {x16, 0x88}, {x17, 0x90}, {x18, 0x98}, {x19, 0xA0}, {x20, 0xA8}, {x21, 0xB0}, {x22, 0xB8}, {x23, 0xC0}, {x24, 0xC8}, {x25, 0xD0}, {x26, 0xD8}, {x27, 0xE0}, {x28, 0xE8}, {x29, 0xF0}, {lr, 0xF8}, {d0, 0x110}, {d1, 0x120}, {d2, 0x130}, {d3, 0x140}, {d4, 0x150}, {d5, 0x160}, {d6, 0x170}, {d7, 0x180}, {d8, 0x190}, {d9, 0x1a0}, {d10, 0x1b0}, {d11, 0x1c0}, {d12, 0x1d0}, {d13, 0x1e0}, {d14, 0x1f0}, {d15, 0x200}, {-1,-1} }},
        { 0x08,  0x00,  0 ,    0x0118,  0x120, FALSE, { {x0, 0x18}, {x1, 0x20}, {x2, 0x28}, {x3, 0x30}, {x4, 0x38}, {x5, 0x40}, {x6, 0x48}, {x7, 0x50}, {x8, 0x58}, {x9, 0x60}, {x10, 0x68}, {x11, 0x70}, {x12, 0x78}, {x13, 0x80}, {x14, 0x88}, {x15, 0x90}, {x16, 0x98}, {x17, 0xA0}, {x18, 0xA8}, {x19, 0xB0}, {x20, 0xB8}, {x21, 0xC0}, {x22, 0xC8}, {x23, 0xD0}, {x24, 0xD8}, {x25, 0xE0}, {x26, 0xE8}, {x27, 0xF0}, {x28, 0xF8}, {x29, 0x100}, {lr, 0x108}, {d0, 0x120}, {d1, 0x130}, {d2, 0x140}, {d3, 0x150}, {d4, 0x160}, {d5, 0x170}, {d6, 0x180}, {d7, 0x190}, {d8, 0x1a0}, {d9, 0x1b0}, {d10, 0x1c0}, {d11, 0x1d0}, {d12, 0x1e0}, {d13, 0x1f0}, {d14, 0x200}, {d15, 0x210}, {-1,-1} }},
#endif
        { 0x0c,  0x00,  0 ,    0x0118,  0x120, FALSE, { {x0, 0x18}, {x1, 0x20}, {x2, 0x28}, {x3, 0x30}, {x4, 0x38}, {x5, 0x40}, {x6, 0x48}, {x7, 0x50}, {x8, 0x58}, {x9, 0x60}, {x10, 0x68}, {x11, 0x70}, {x12, 0x78}, {x13, 0x80}, {x14, 0x88}, {x15, 0x90}, {x16, 0x98}, {x17, 0xA0}, {x18, 0xA8}, {x19, 0xB0}, {x20, 0xB8}, {x21, 0xC0}, {x22, 0xC8}, {x23, 0xD0}, {x24, 0xD8}, {x25, 0xE0}, {x26, 0xE8}, {x27, 0xF0}, {x28, 0xF8}, {x29, 0x100}, {lr, 0x108}, {d0, 0x120}, {d1, 0x130}, {d2, 0x140}, {d3, 0x150}, {d4, 0x160}, {d5, 0x170}, {d6, 0x180}, {d7, 0x190}, {d8, 0x1a0}, {d9, 0x1b0}, {d10, 0x1c0}, {d11, 0x1d0}, {d12, 0x1e0}, {d13, 0x1f0}, {d14, 0x200}, {d15, 0x210}, {-1,-1} }},
    };

    static const BYTE function_4[] =
    {
        0xff, 0x43, 0x00, 0xd1,   /* 00: sub sp,  sp,  #16 */
        0xff, 0x03, 0x08, 0xd1,   /* 04: sub sp,  sp,  #512 */
        0xff, 0x43, 0x40, 0xd1,   /* 08: sub sp,  sp,  #65536 */
        0xfd, 0x03, 0x00, 0x91,   /* 0c: mov x29, sp */
        0xf3, 0x53, 0xbe, 0xa9,   /* 10: stp x19, x20, [sp, #-32]! */
        0xf5, 0x5b, 0x01, 0xa9,   /* 14: stp x21, x22, [sp, #16] */
        0xf7, 0x0f, 0x1e, 0xf8,   /* 18: str x23,      [sp, #-32]! */
        0xf8, 0x07, 0x00, 0xf9,   /* 1c: str x24,      [sp, #8] */
        0xf9, 0x7b, 0x01, 0xa9,   /* 20: stp x25, x30, [sp, #16] */
        0xfd, 0x7b, 0x03, 0xa9,   /* 24: stp x29, x30, [sp, #48] */
        0xfd, 0x7b, 0xbe, 0xa9,   /* 28: stp x29, x30, [sp, #-32]! */
        0xf3, 0x53, 0xbe, 0xa9,   /* 2c: stp x19, x20, [sp, #-32]! */
        0xe8, 0x27, 0xbe, 0x6d,   /* 30: stp d8,  d9,  [sp, #-32]! */
        0xea, 0x2f, 0x01, 0x6d,   /* 34: stp d10, d11, [sp, #16] */
        0xec, 0x0f, 0x1e, 0xfc,   /* 38: str d12,      [sp, #-32]! */
        0xed, 0x07, 0x00, 0xfd,   /* 3c: str d13,      [sp, #8] */
        0xfd, 0x43, 0x00, 0x91,   /* 40: add x29, sp,  #16 */
        0xc0, 0x03, 0x5f, 0xd6,   /* 44: ret */
    };

    static const DWORD unwind_info_4_header =
        (sizeof(function_4)/4) | /* function length */
        (0 << 20) | /* X */
        (0 << 21) | /* E */
        (0 << 22) | /* epilog */
        (8 << 27);  /* codes */

    static const BYTE unwind_info_4[] =
    {
        DW(unwind_info_4_header),

        UWOP_ADD_FP(16),          /* 40: add x29, sp, #16 */
        UWOP_SAVE_FREG(13, 8),    /* 3c: str d13,      [sp, #8] */
        UWOP_SAVE_FREG_X(12, 32), /* 38: str d12,      [sp, #-32]! */
        UWOP_SAVE_FREGP(10, 16),  /* 34: stp d10, d11, [sp, #16] */
        UWOP_SAVE_FREGP_X(8, 32), /* 30: stp d8,  d9,  [sp, #-32]! */
        UWOP_SAVE_R19R20_X(32),   /* 2c: stp x19, x20, [sp, #-32]! */
        UWOP_SAVE_FPLR_X(32),     /* 28: stp x29, x30, [sp, #-32]! */
        UWOP_SAVE_FPLR(16),       /* 24: stp x29, x30, [sp, #16] */
        UWOP_SAVE_LRP(25, 16),    /* 20: stp x25, x30, [sp, #16] */
        UWOP_SAVE_REG(24, 8),     /* 1c: str x24,      [sp, #8] */
        UWOP_SAVE_REG_X(23, 32),  /* 18: str x23,      [sp, #-32]! */
        UWOP_SAVE_REGP(21, 16),   /* 14: stp x21, x22, [sp, #16] */
        UWOP_SAVE_REGP_X(19, 32), /* 10: stp x19, x20, [sp, #-32]! */
        UWOP_SET_FP,              /* 0c: mov x29, sp */
        UWOP_ALLOC_LARGE(65536),  /* 08: sub sp,  sp,  #65536 */
        UWOP_ALLOC_MEDIUM(512),   /* 04: sub sp,  sp,  #512 */
        UWOP_ALLOC_SMALL(16),     /* 00: sub sp,  sp,  #16 */
        UWOP_END,
    };

    static const struct results results_4[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x00000, TRUE, { {-1,-1} }},
        { 0x04,  0x10,  0,     ORIG_LR, 0x00010, TRUE, { {-1,-1} }},
        { 0x08,  0x10,  0,     ORIG_LR, 0x00210, TRUE, { {-1,-1} }},
        { 0x0c,  0x10,  0,     ORIG_LR, 0x10210, TRUE, { {-1,-1} }},
        { 0x14,  0x00,  0,     ORIG_LR, 0x10210, TRUE, { {x19, 0x00}, {x20, 0x08}, {-1,-1} }},
        { 0x18,  0x00,  0,     ORIG_LR, 0x10210, TRUE, { {x19, 0x00}, {x20, 0x08}, {x21, 0x10}, {x22, 0x18}, {-1,-1} }},
        { 0x1c,  0x00,  0,     ORIG_LR, 0x10210, TRUE, { {x19, 0x20}, {x20, 0x28}, {x21, 0x30}, {x22, 0x38}, {x23, 0x00}, {-1,-1} }},
        { 0x20,  0x00,  0,     ORIG_LR, 0x10210, TRUE, { {x19, 0x20}, {x20, 0x28}, {x21, 0x30}, {x22, 0x38}, {x23, 0x00}, {x24, 0x08}, {-1,-1} }},
        { 0x24,  0x00,  0,     0x0018,  0x10210, TRUE, { {x19, 0x20}, {x20, 0x28}, {x21, 0x30}, {x22, 0x38}, {x23, 0x00}, {x24, 0x08}, {x25, 0x10}, {lr, 0x18}, {-1,-1} }},
        { 0x28,  0x00,  0,     0x0018,  0x10220, FALSE, { {x19, 0x20}, {x20, 0x28}, {x21, 0x30}, {x22, 0x38}, {x23, 0x00}, {x24, 0x08}, {x25, 0x10}, {lr, 0x18}, {x29, 0x10}, {-1,-1} }},
        { 0x2c,  0x00,  0,     0x0038,  0x10240, FALSE, { {x19, 0x40}, {x20, 0x48}, {x21, 0x50}, {x22, 0x58}, {x23, 0x20}, {x24, 0x28}, {x25, 0x30}, {lr, 0x38}, {x29, 0x30}, {-1,-1} }},
        { 0x30,  0x00,  0,     0x0058,  0x10260, FALSE, { {x19, 0x60}, {x20, 0x68}, {x21, 0x70}, {x22, 0x78}, {x23, 0x40}, {x24, 0x48}, {x25, 0x50}, {lr, 0x58}, {x29, 0x50}, {-1,-1} }},
        { 0x34,  0x00,  0,     0x0078,  0x10280, FALSE, { {x19, 0x80}, {x20, 0x88}, {x21, 0x90}, {x22, 0x98}, {x23, 0x60}, {x24, 0x68}, {x25, 0x70}, {lr, 0x78}, {x29, 0x70}, {d8, 0x00}, {d9, 0x08}, {-1,-1} }},
        { 0x38,  0x00,  0,     0x0078,  0x10280, FALSE, { {x19, 0x80}, {x20, 0x88}, {x21, 0x90}, {x22, 0x98}, {x23, 0x60}, {x24, 0x68}, {x25, 0x70}, {lr, 0x78}, {x29, 0x70}, {d8, 0x00}, {d9, 0x08}, {d10, 0x10}, {d11, 0x18}, {-1,-1} }},
        { 0x3c,  0x00,  0,     0x0098,  0x102a0, FALSE, { {x19, 0xa0}, {x20, 0xa8}, {x21, 0xb0}, {x22, 0xb8}, {x23, 0x80}, {x24, 0x88}, {x25, 0x90}, {lr, 0x98}, {x29, 0x90}, {d8, 0x20}, {d9, 0x28}, {d10, 0x30}, {d11, 0x38}, {d12, 0x00}, {-1,-1} }},
        { 0x40,  0x00,  0,     0x0098,  0x102a0, FALSE, { {x19, 0xa0}, {x20, 0xa8}, {x21, 0xb0}, {x22, 0xb8}, {x23, 0x80}, {x24, 0x88}, {x25, 0x90}, {lr, 0x98}, {x29, 0x90}, {d8, 0x20}, {d9, 0x28}, {d10, 0x30}, {d11, 0x38}, {d12, 0x00}, {d13, 0x08}, {-1,-1} }},
        { 0x44,  0x20,  0,     0x00a8,  0x102b0, FALSE, { {x19, 0xb0}, {x20, 0xb8}, {x21, 0xc0}, {x22, 0xc8}, {x23, 0x90}, {x24, 0x98}, {x25, 0xa0}, {lr, 0xa8}, {x29, 0xa0}, {d8, 0x30}, {d9, 0x38}, {d10, 0x40}, {d11, 0x48}, {d12, 0x10}, {d13, 0x18}, {-1,-1} }},
    };

    static const BYTE function_5[] =
    {
        0xf3, 0x53, 0xbe, 0xa9,   /* 00: stp x19, x20, [sp, #-32]! */
        0xf5, 0x5b, 0x01, 0xa9,   /* 04: stp x21, x22, [sp, #16] */
        0xf7, 0x63, 0xbc, 0xa9,   /* 08: stp x23, x24, [sp, #-64]! */
        0xf9, 0x6b, 0x01, 0xa9,   /* 0c: stp x25, x26, [sp, #16] */
        0xfb, 0x73, 0x02, 0xa9,   /* 10: stp x27, x28, [sp, #32] */
        0xfd, 0x7b, 0x03, 0xa9,   /* 14: stp x29, x30, [sp, #48] */
        0xe8, 0x27, 0xbc, 0x6d,   /* 18: stp d8,  d9,  [sp, #-64]! */
        0xea, 0x2f, 0x01, 0x6d,   /* 1c: stp d10, d11, [sp, #16] */
        0xec, 0x37, 0x02, 0x6d,   /* 20: stp d12, d13, [sp, #32] */
        0xee, 0x3f, 0x03, 0x6d,   /* 24: stp d14, d15, [sp, #48] */
        0xc0, 0x03, 0x5f, 0xd6,   /* 28: ret */
    };

    static const DWORD unwind_info_5_header =
        (sizeof(function_5)/4) | /* function length */
        (0 << 20) | /* X */
        (0 << 21) | /* E */
        (0 << 22) | /* epilog */
        (4 << 27);  /* codes */

    static const BYTE unwind_info_5[] =
    {
        DW(unwind_info_5_header),

        UWOP_SAVE_NEXT,           /* 24: stp d14, d15, [sp, #48] */
        UWOP_SAVE_FREGP(12, 32),  /* 20: stp d12, d13, [sp, #32] */
        UWOP_SAVE_NEXT,           /* 1c: stp d10, d11, [sp, #16] */
        UWOP_SAVE_FREGP_X(8, 64), /* 18: stp d8,  d9,  [sp, #-64]! */
        UWOP_SAVE_NEXT,           /* 14: stp x29, x30, [sp, #48] */
        UWOP_SAVE_REGP(27, 32),   /* 10: stp x27, x28, [sp, #32] */
        UWOP_SAVE_NEXT,           /* 0c: stp x25, x26, [sp, #16] */
        UWOP_SAVE_REGP_X(23, 64), /* 08: stp x23, x24, [sp, #-64]! */
        UWOP_SAVE_NEXT,           /* 04: stp x21, x22, [sp, #16] */
        UWOP_SAVE_R19R20_X(32),   /* 00: stp x19, x20, [sp, #-32]! */
        UWOP_END,
        UWOP_NOP                  /* padding */
    };

    /* Windows seems to only save one register for UWOP_SAVE_NEXT for
     * float registers, contrary to what the documentation says. The tests
     * for those cases are commented out; they succeed in wine but fail
     * on native windows. */
    static const struct results results_5[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x00000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     ORIG_LR, 0x00020, TRUE, { {x19, 0x00}, {x20, 0x08}, {-1,-1} }},
        { 0x08,  0x00,  0,     ORIG_LR, 0x00020, TRUE, { {x19, 0x00}, {x20, 0x08}, {x21, 0x10}, {x22, 0x18}, {-1,-1} }},
        { 0x0c,  0x00,  0,     ORIG_LR, 0x00060, TRUE, { {x19, 0x40}, {x20, 0x48}, {x21, 0x50}, {x22, 0x58}, {x23, 0x00}, {x24, 0x08}, {-1,-1} }},
        { 0x10,  0x00,  0,     ORIG_LR, 0x00060, TRUE, { {x19, 0x40}, {x20, 0x48}, {x21, 0x50}, {x22, 0x58}, {x23, 0x00}, {x24, 0x08}, {x25, 0x10}, {x26, 0x18}, {-1,-1} }},
        { 0x14,  0x00,  0,     ORIG_LR, 0x00060, TRUE, { {x19, 0x40}, {x20, 0x48}, {x21, 0x50}, {x22, 0x58}, {x23, 0x00}, {x24, 0x08}, {x25, 0x10}, {x26, 0x18}, {x27, 0x20}, {x28, 0x28}, {-1,-1} }},
        { 0x18,  0x00,  0,     0x38,    0x00060, TRUE, { {x19, 0x40}, {x20, 0x48}, {x21, 0x50}, {x22, 0x58}, {x23, 0x00}, {x24, 0x08}, {x25, 0x10}, {x26, 0x18}, {x27, 0x20}, {x28, 0x28}, {x29, 0x30}, {lr, 0x38}, {-1,-1} }},
        { 0x1c,  0x00,  0,     0x78,    0x000a0, TRUE, { {x19, 0x80}, {x20, 0x88}, {x21, 0x90}, {x22, 0x98}, {x23, 0x40}, {x24, 0x48}, {x25, 0x50}, {x26, 0x58}, {x27, 0x60}, {x28, 0x68}, {x29, 0x70}, {lr, 0x78}, {d8, 0x00}, {d9, 0x08}, {-1,-1} }},
#if 0
        { 0x20,  0x00,  0,     0x78,    0x000a0, TRUE, { {x19, 0x80}, {x20, 0x88}, {x21, 0x90}, {x22, 0x98}, {x23, 0x40}, {x24, 0x48}, {x25, 0x50}, {x26, 0x58}, {x27, 0x60}, {x28, 0x68}, {x29, 0x70}, {lr, 0x78}, {d8, 0x00}, {d9, 0x08}, {d10, 0x10}, {d11, 0x18}, {-1,-1} }},
        { 0x24,  0x00,  0,     0x78,    0x000a0, TRUE, { {x19, 0x80}, {x20, 0x88}, {x21, 0x90}, {x22, 0x98}, {x23, 0x40}, {x24, 0x48}, {x25, 0x50}, {x26, 0x58}, {x27, 0x60}, {x28, 0x68}, {x29, 0x70}, {lr, 0x78}, {d8, 0x00}, {d9, 0x08}, {d10, 0x10}, {d11, 0x18}, {d12, 0x20}, {d13, 0x28}, {-1,-1} }},
        { 0x28,  0x00,  0,     0x78,    0x000a0, TRUE, { {x19, 0x80}, {x20, 0x88}, {x21, 0x90}, {x22, 0x98}, {x23, 0x40}, {x24, 0x48}, {x25, 0x50}, {x26, 0x58}, {x27, 0x60}, {x28, 0x68}, {x29, 0x70}, {lr, 0x78}, {d8, 0x00}, {d9, 0x08}, {d10, 0x10}, {d11, 0x18}, {d12, 0x20}, {d13, 0x28}, {d14, 0x30}, {d15, 0x38}, {-1,-1} }},
#endif
    };

    static const BYTE function_6[] =
    {
        0xf3, 0x53, 0xbd, 0xa9,   /* 00: stp x19, x20, [sp, #-48]! */
        0xf5, 0x0b, 0x00, 0xf9,   /* 04: str x21,      [sp, #16] */
        0xe8, 0xa7, 0x01, 0x6d,   /* 08: stp d8,  d9,  [sp, #24] */
        0xea, 0x17, 0x00, 0xfd,   /* 0c: str d10,      [sp, #40] */
        0xff, 0x03, 0x00, 0xd1,   /* 10: sub sp,  sp,  #0 */
        0x1f, 0x20, 0x03, 0xd5,   /* 14: nop */
        0xff, 0x03, 0x00, 0x91,   /* 18: add sp,  sp,  #0 */
        0xea, 0x17, 0x40, 0xfd,   /* 1c: ldr d10,      [sp, #40] */
        0xe8, 0xa7, 0x41, 0x6d,   /* 20: ldp d8,  d9,  [sp, #24] */
        0xf5, 0x0b, 0x40, 0xf9,   /* 24: ldr x21,      [sp, #16] */
        0xf3, 0x53, 0xc3, 0xa8,   /* 28: ldp x19, x20, [sp], #48 */
        0xc0, 0x03, 0x5f, 0xd6,   /* 2c: ret */
    };

    static const DWORD unwind_info_6_packed =
        (1 << 0)  | /* Flag */
        (sizeof(function_6)/4 << 2) | /* FunctionLength */
        (2 << 13) | /* RegF */
        (3 << 16) | /* RegI */
        (0 << 20) | /* H */
        (0 << 21) | /* CR */
        (3 << 23);  /* FrameSize */

    static const BYTE unwind_info_6[] = { DW(unwind_info_6_packed) };

    static const struct results results_6[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19,0x00}, {x20,0x08}, {-1,-1} }},
        { 0x08,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19,0x00}, {x20,0x08}, {x21, 0x10}, {-1,-1} }},
        { 0x0c,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19,0x00}, {x20,0x08}, {x21, 0x10}, {d8, 0x18}, {d9, 0x20}, {-1,-1} }},
        { 0x10,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19,0x00}, {x20,0x08}, {x21, 0x10}, {d8, 0x18}, {d9, 0x20}, {d10, 0x28}, {-1,-1} }},
        { 0x14,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19,0x00}, {x20,0x08}, {x21, 0x10}, {d8, 0x18}, {d9, 0x20}, {d10, 0x28}, {-1,-1} }},
        { 0x18,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19,0x00}, {x20,0x08}, {x21, 0x10}, {d8, 0x18}, {d9, 0x20}, {d10, 0x28}, {-1,-1} }},
        { 0x1c,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19,0x00}, {x20,0x08}, {x21, 0x10}, {d8, 0x18}, {d9, 0x20}, {d10, 0x28}, {-1,-1} }},
        { 0x20,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19,0x00}, {x20,0x08}, {x21, 0x10}, {d8, 0x18}, {d9, 0x20}, {-1,-1} }},
        { 0x24,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19,0x00}, {x20,0x08}, {x21, 0x10}, {-1,-1} }},
        { 0x28,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19,0x00}, {x20,0x08}, {-1,-1} }},
        { 0x2c,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_7[] =
    {
        0xf3, 0x0f, 0x1d, 0xf8,   /* 00: str x19,      [sp, #-48]! */
        0xe8, 0xa7, 0x00, 0x6d,   /* 04: stp d8,  d9,  [sp, #8] */
        0xea, 0xaf, 0x01, 0x6d,   /* 08: stp d10, d11, [sp, #24] */
        0xff, 0x03, 0x00, 0xd1,   /* 0c: sub sp,  sp,  #0 */
        0x1f, 0x20, 0x03, 0xd5,   /* 10: nop */
        0xff, 0x03, 0x00, 0x91,   /* 14: add sp,  sp,  #0 */
        0xea, 0xaf, 0x41, 0x6d,   /* 18: ldp d10, d11, [sp, #24] */
        0xe8, 0xa7, 0x40, 0x6d,   /* 1c: ldp d8,  d9,  [sp, #8] */
        0xf3, 0x07, 0x43, 0xf8,   /* 20: ldr x19,      [sp], #48 */
        0xc0, 0x03, 0x5f, 0xd6,   /* 24: ret */
    };

    static const DWORD unwind_info_7_packed =
        (1 << 0)  | /* Flag */
        (sizeof(function_7)/4 << 2) | /* FunctionLength */
        (3 << 13) | /* RegF */
        (1 << 16) | /* RegI */
        (0 << 20) | /* H */
        (0 << 21) | /* CR */
        (3 << 23);  /* FrameSize */

    static const BYTE unwind_info_7[] = { DW(unwind_info_7_packed) };

    static const struct results results_7[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19, 0x00}, {-1,-1} }},
        { 0x08,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19, 0x00}, {d8, 0x08}, {d9, 0x10}, {-1,-1} }},
        { 0x0c,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19, 0x00}, {d8, 0x08}, {d9, 0x10}, {d10, 0x18}, {d11, 0x20}, {-1,-1} }},
        { 0x10,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19, 0x00}, {d8, 0x08}, {d9, 0x10}, {d10, 0x18}, {d11, 0x20}, {-1,-1} }},
        { 0x14,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19, 0x00}, {d8, 0x08}, {d9, 0x10}, {d10, 0x18}, {d11, 0x20}, {-1,-1} }},
        { 0x18,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19, 0x00}, {d8, 0x08}, {d9, 0x10}, {d10, 0x18}, {d11, 0x20}, {-1,-1} }},
        { 0x1c,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19, 0x00}, {d8, 0x08}, {d9, 0x10}, {-1,-1} }},
        { 0x20,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19, 0x00}, {-1,-1} }},
        { 0x24,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_8[] =
    {
        0xe8, 0x27, 0xbf, 0x6d,   /* 00: stp d8,  d9,  [sp, #-16]! */
        0xff, 0x83, 0x00, 0xd1,   /* 04: sub sp,  sp,  #32 */
        0x1f, 0x20, 0x03, 0xd5,   /* 08: nop */
        0xff, 0x83, 0x00, 0x91,   /* 0c: add sp,  sp,  #32 */
        0xe8, 0x27, 0xc1, 0x6c,   /* 10: ldp d8,  d9,  [sp], #16 */
        0xc0, 0x03, 0x5f, 0xd6,   /* 14: ret */
    };

    static const DWORD unwind_info_8_packed =
        (1 << 0)  | /* Flag */
        (sizeof(function_8)/4 << 2) | /* FunctionLength */
        (1 << 13) | /* RegF */
        (0 << 16) | /* RegI */
        (0 << 20) | /* H */
        (0 << 21) | /* CR */
        (3 << 23);  /* FrameSize */

    static const BYTE unwind_info_8[] = { DW(unwind_info_8_packed) };

    static const struct results results_8[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     ORIG_LR, 0x010, TRUE, { {d8, 0x00}, {d9, 0x08}, {-1,-1} }},
        { 0x08,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {d8, 0x20}, {d9, 0x28}, {-1,-1} }},
        { 0x0c,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {d8, 0x20}, {d9, 0x28}, {-1,-1} }},
        { 0x10,  0x00,  0,     ORIG_LR, 0x010, TRUE, { {d8, 0x00}, {d9, 0x08}, {-1,-1} }},
        { 0x14,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_9[] =
    {
        0xf3, 0x0f, 0x1b, 0xf8,   /* 00: str x19,      [sp, #-80]! */
        0xe0, 0x87, 0x00, 0xa9,   /* 04: stp x0,  x1,  [sp, #8] */
        0xe2, 0x8f, 0x01, 0xa9,   /* 08: stp x2,  x3,  [sp, #24] */
        0xe4, 0x97, 0x02, 0xa9,   /* 0c: stp x4,  x5,  [sp, #40] */
        0xe6, 0x9f, 0x03, 0xa9,   /* 10: stp x6,  x7,  [sp, #56] */
        0xff, 0x83, 0x00, 0xd1,   /* 14: sub sp,  sp,  #32 */
        0x1f, 0x20, 0x03, 0xd5,   /* 18: nop */
        0xff, 0x83, 0x00, 0x91,   /* 1c: add sp,  sp,  #32 */
        0x1f, 0x20, 0x03, 0xd5,   /* 20: nop */
        0x1f, 0x20, 0x03, 0xd5,   /* 24: nop */
        0x1f, 0x20, 0x03, 0xd5,   /* 28: nop */
        0x1f, 0x20, 0x03, 0xd5,   /* 2c: nop */
        0xf3, 0x0f, 0x1b, 0xf8,   /* 30: ldr x19,      [sp], #80 */
        0xc0, 0x03, 0x5f, 0xd6,   /* 34: ret */
    };

    static const DWORD unwind_info_9_packed =
        (1 << 0)  | /* Flag */
        (sizeof(function_9)/4 << 2) | /* FunctionLength */
        (0 << 13) | /* RegF */
        (1 << 16) | /* RegI */
        (1 << 20) | /* H */
        (0 << 21) | /* CR */
        (7 << 23);  /* FrameSize */

    static const BYTE unwind_info_9[] = { DW(unwind_info_9_packed) };

    static const struct results results_9[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     ORIG_LR, 0x050, TRUE, { {x19, 0x00}, {-1,-1} }},
        { 0x08,  0x00,  0,     ORIG_LR, 0x050, TRUE, { {x19, 0x00}, {-1,-1} }},
        { 0x0c,  0x00,  0,     ORIG_LR, 0x050, TRUE, { {x19, 0x00}, {-1,-1} }},
        { 0x10,  0x00,  0,     ORIG_LR, 0x050, TRUE, { {x19, 0x00}, {-1,-1} }},
        { 0x14,  0x00,  0,     ORIG_LR, 0x050, TRUE, { {x19, 0x00}, {-1,-1} }},
        { 0x18,  0x00,  0,     ORIG_LR, 0x070, TRUE, { {x19, 0x20}, {-1,-1} }},
        { 0x1c,  0x00,  0,     ORIG_LR, 0x070, TRUE, { {x19, 0x20}, {-1,-1} }},
        { 0x20,  0x00,  0,     ORIG_LR, 0x050, TRUE, { {x19, 0x00}, {-1,-1} }},
        { 0x24,  0x00,  0,     ORIG_LR, 0x050, TRUE, { {x19, 0x00}, {-1,-1} }},
        { 0x28,  0x00,  0,     ORIG_LR, 0x050, TRUE, { {x19, 0x00}, {-1,-1} }},
        { 0x2c,  0x00,  0,     ORIG_LR, 0x050, TRUE, { {x19, 0x00}, {-1,-1} }},
        { 0x30,  0x00,  0,     ORIG_LR, 0x050, TRUE, { {x19, 0x00}, {-1,-1} }},
        { 0x34,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_10[] =
    {
        0xfe, 0x0f, 0x1f, 0xf8,   /* 00: str lr,       [sp, #-16]! */
        0xff, 0x43, 0x00, 0xd1,   /* 04: sub sp,  sp,  #16 */
        0x1f, 0x20, 0x03, 0xd5,   /* 08: nop */
        0xff, 0x43, 0x00, 0x91,   /* 0c: add sp,  sp,  #16 */
        0xfe, 0x07, 0x41, 0xf8,   /* 10: ldr lr,       [sp], #16 */
        0xc0, 0x03, 0x5f, 0xd6,   /* 14: ret */
    };

    static const DWORD unwind_info_10_packed =
        (1 << 0)  | /* Flag */
        (sizeof(function_10)/4 << 2) | /* FunctionLength */
        (0 << 13) | /* RegF */
        (0 << 16) | /* RegI */
        (0 << 20) | /* H */
        (1 << 21) | /* CR */
        (2 << 23);  /* FrameSize */

    static const BYTE unwind_info_10[] = { DW(unwind_info_10_packed) };

    static const struct results results_10[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     0x00,    0x010, TRUE, { {lr, 0x00}, {-1,-1} }},
        { 0x08,  0x00,  0,     0x10,    0x020, TRUE, { {lr, 0x10}, {-1,-1} }},
        { 0x0c,  0x00,  0,     0x10,    0x020, TRUE, { {lr, 0x10}, {-1,-1} }},
        { 0x10,  0x00,  0,     0x00,    0x010, TRUE, { {lr, 0x00}, {-1,-1} }},
        { 0x14,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_11[] =
    {
        0xf3, 0x53, 0xbe, 0xa9,   /* 00: stp x19, x20, [sp, #-32]! */
        0xf5, 0x7b, 0x01, 0xa9,   /* 04: stp x21, lr,  [sp, #16] */
        0xff, 0x43, 0x00, 0xd1,   /* 08: sub sp,  sp,  #16 */
        0x1f, 0x20, 0x03, 0xd5,   /* 0c: nop */
        0xff, 0x43, 0x00, 0x91,   /* 10: add sp,  sp,  #16 */
        0xf5, 0x7b, 0x41, 0xa9,   /* 14: ldp x21, lr,  [sp, #16] */
        0xf3, 0x53, 0xc2, 0xa8,   /* 18: ldp x19, x20, [sp], #32 */
        0xc0, 0x03, 0x5f, 0xd6,   /* 1c: ret */
    };

    static const DWORD unwind_info_11_packed =
        (1 << 0)  | /* Flag */
        (sizeof(function_11)/4 << 2) | /* FunctionLength */
        (0 << 13) | /* RegF */
        (3 << 16) | /* RegI */
        (0 << 20) | /* H */
        (1 << 21) | /* CR */
        (3 << 23);  /* FrameSize */

    static const BYTE unwind_info_11[] = { DW(unwind_info_11_packed) };

    static const struct results results_11[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     ORIG_LR, 0x020, TRUE, { {x19, 0x00}, {x20, 0x08}, {-1,-1} }},
        { 0x08,  0x00,  0,     0x18,    0x020, TRUE, { {x19, 0x00}, {x20, 0x08}, {x21, 0x10}, {lr, 0x18}, {-1,-1} }},
        { 0x0c,  0x00,  0,     0x28,    0x030, TRUE, { {x19, 0x10}, {x20, 0x18}, {x21, 0x20}, {lr, 0x28}, {-1,-1} }},
        { 0x10,  0x00,  0,     0x28,    0x030, TRUE, { {x19, 0x10}, {x20, 0x18}, {x21, 0x20}, {lr, 0x28}, {-1,-1} }},
        { 0x14,  0x00,  0,     0x18,    0x020, TRUE, { {x19, 0x00}, {x20, 0x08}, {x21, 0x10}, {lr, 0x18}, {-1,-1} }},
        { 0x18,  0x00,  0,     ORIG_LR, 0x020, TRUE, { {x19, 0x00}, {x20, 0x08}, {-1,-1} }},
        { 0x1c,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_12[] =
    {
        0xf3, 0x53, 0xbf, 0xa9,   /* 00: stp x19, x20, [sp, #-16]! */
        0xfd, 0x7b, 0xbe, 0xa9,   /* 04: stp x29, lr,  [sp, #-32]! */
        0xfd, 0x03, 0x00, 0x91,   /* 08: mov x29, sp */
        0x1f, 0x20, 0x03, 0xd5,   /* 0c: nop */
        0xbf, 0x03, 0x00, 0x91,   /* 10: mov sp,  x29 */
        0xfd, 0x7b, 0xc2, 0xa8,   /* 14: ldp x29, lr,  [sp], #32 */
        0xf3, 0x53, 0xc1, 0xa8,   /* 18: ldp x19, x20, [sp], #16 */
        0xc0, 0x03, 0x5f, 0xd6,   /* 1c: ret */
    };

    static const DWORD unwind_info_12_packed =
        (1 << 0)  | /* Flag */
        (sizeof(function_12)/4 << 2) | /* FunctionLength */
        (0 << 13) | /* RegF */
        (2 << 16) | /* RegI */
        (0 << 20) | /* H */
        (3 << 21) | /* CR */
        (3 << 23);  /* FrameSize */

    static const BYTE unwind_info_12[] = { DW(unwind_info_12_packed) };

    static const struct results results_12[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x10,  0,     ORIG_LR, 0x010, TRUE, { {x19, 0x00}, {x20, 0x08}, {-1,-1} }},
        { 0x08,  0x10,  0,     0x08,    0x030, TRUE, { {x19, 0x20}, {x20, 0x28}, {x29, 0x00}, {lr, 0x08}, {-1,-1} }},
        { 0x0c,  0x10,  0,     0x18,    0x040, TRUE, { {x19, 0x30}, {x20, 0x38}, {x29, 0x10}, {lr, 0x18}, {-1,-1} }},
        { 0x10,  0x10,  0,     0x18,    0x040, TRUE, { {x19, 0x30}, {x20, 0x38}, {x29, 0x10}, {lr, 0x18}, {-1,-1} }},
        { 0x14,  0x10,  0,     0x08,    0x030, TRUE, { {x19, 0x20}, {x20, 0x28}, {x29, 0x00}, {lr, 0x08}, {-1,-1} }},
        { 0x18,  0x10,  0,     ORIG_LR, 0x010, TRUE, { {x19, 0x00}, {x20, 0x08}, {-1,-1} }},
        { 0x1c,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_13[] =
    {
        0xf3, 0x53, 0xbf, 0xa9,   /* 00: stp x19, x20, [sp, #-16]! */
        0xff, 0x43, 0x08, 0xd1,   /* 04: sub sp,  sp,  #528 */
        0xfd, 0x7b, 0x00, 0xd1,   /* 08: stp x29, lr,  [sp] */
        0xfd, 0x03, 0x00, 0x91,   /* 0c: mov x29, sp */
        0x1f, 0x20, 0x03, 0xd5,   /* 10: nop */
        0xbf, 0x03, 0x00, 0x91,   /* 14: mov sp,  x29 */
        0xfd, 0x7b, 0x40, 0xa9,   /* 18: ldp x29, lr,  [sp] */
        0xff, 0x43, 0x08, 0x91,   /* 1c: add sp,  sp,  #528 */
        0xf3, 0x53, 0xc1, 0xa8,   /* 20: ldp x19, x20, [sp], #16 */
        0xc0, 0x03, 0x5f, 0xd6,   /* 24: ret */
    };

    static const DWORD unwind_info_13_packed =
        (1 << 0)  | /* Flag */
        (sizeof(function_13)/4 << 2) | /* FunctionLength */
        (0 << 13) | /* RegF */
        (2 << 16) | /* RegI */
        (0 << 20) | /* H */
        (3 << 21) | /* CR */
        (34 << 23);  /* FrameSize */

    static const BYTE unwind_info_13[] = { DW(unwind_info_13_packed) };

    static const struct results results_13[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x10,  0,     ORIG_LR, 0x010, TRUE, { {x19, 0x00}, {x20, 0x08}, {-1,-1} }},
        { 0x08,  0x10,  0,     ORIG_LR, 0x220, TRUE, { {x19, 0x210}, {x20, 0x218}, {-1,-1} }},
        { 0x0c,  0x10,  0,     0x08,    0x220, TRUE, { {x19, 0x210}, {x20, 0x218}, {x29, 0x00}, {lr, 0x08}, {-1,-1} }},
        { 0x10,  0x10,  0,     0x18,    0x230, TRUE, { {x19, 0x220}, {x20, 0x228}, {x29, 0x10}, {lr, 0x18}, {-1,-1} }},
        { 0x14,  0x10,  0,     0x18,    0x230, TRUE, { {x19, 0x220}, {x20, 0x228}, {x29, 0x10}, {lr, 0x18}, {-1,-1} }},
        { 0x18,  0x10,  0,     0x08,    0x220, TRUE, { {x19, 0x210}, {x20, 0x218}, {x29, 0x00}, {lr, 0x08}, {-1,-1} }},
        { 0x1c,  0x10,  0,     ORIG_LR, 0x220, TRUE, { {x19, 0x210}, {x20, 0x218}, {-1,-1} }},
        { 0x20,  0x10,  0,     ORIG_LR, 0x010, TRUE, { {x19, 0x00}, {x20, 0x08}, {-1,-1} }},
        { 0x24,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const struct unwind_test tests[] =
    {
#define TEST(func, unwind, unwind_packed, results) \
        { func, sizeof(func), unwind, unwind_packed ? 0 : sizeof(unwind), results, ARRAY_SIZE(results) }
        TEST(function_0, unwind_info_0, 0, results_0),
        TEST(function_1, unwind_info_1, 1, results_1),
        TEST(function_2, unwind_info_2, 0, results_2),
        TEST(function_3, unwind_info_3, 0, results_3),
        TEST(function_4, unwind_info_4, 0, results_4),
        TEST(function_5, unwind_info_5, 0, results_5),
        TEST(function_6, unwind_info_6, 1, results_6),
        TEST(function_7, unwind_info_7, 1, results_7),
        TEST(function_8, unwind_info_8, 1, results_8),
        TEST(function_9, unwind_info_9, 1, results_9),
        TEST(function_10, unwind_info_10, 1, results_10),
        TEST(function_11, unwind_info_11, 1, results_11),
        TEST(function_12, unwind_info_12, 1, results_12),
        TEST(function_13, unwind_info_13, 1, results_13),
#undef TEST
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
        call_virtual_unwind( i, &tests[i] );
}

static void test_thread_context(void)
{
    CONTEXT context;
    NTSTATUS status;
    struct expected
    {
        ULONG64 X0, X1, X2, X3, X4, X5, X6, X7, X8, X9, X10, X11, X12, X13, X14, X15, X16,
            X17, X18, X19, X20, X21, X22, X23, X24, X25, X26, X27, X28, Fp, Lr, Sp, Pc;
        ULONG Cpsr, Fpcr, Fpsr;
    } expect;
    NTSTATUS (*func_ptr)( void *arg1, void *arg2, struct expected *res, void *func ) = code_mem;

    static const DWORD call_func[] =
    {
        0xa9bf7bfd,  /* stp     x29, x30, [sp, #-16]! */
        0xa9000440,  /* stp     x0, x1, [x2] */
        0xa9010c42,  /* stp     x2, x3, [x2, #16] */
        0xa9021444,  /* stp     x4, x5, [x2, #32] */
        0xa9031c46,  /* stp     x6, x7, [x2, #48] */
        0xa9042448,  /* stp     x8, x9, [x2, #64] */
        0xa9052c4a,  /* stp     x10, x11, [x2, #80] */
        0xa906344c,  /* stp     x12, x13, [x2, #96] */
        0xa9073c4e,  /* stp     x14, x15, [x2, #112] */
        0xa9084450,  /* stp     x16, x17, [x2, #128] */
        0xa9094c52,  /* stp     x18, x19, [x2, #144] */
        0xa90a5454,  /* stp     x20, x21, [x2, #160] */
        0xa90b5c56,  /* stp     x22, x23, [x2, #176] */
        0xa90c6458,  /* stp     x24, x25, [x2, #192] */
        0xa90d6c5a,  /* stp     x26, x27, [x2, #208] */
        0xa90e745c,  /* stp     x28, x29, [x2, #224] */
        0xf900785e,  /* str     x30, [x2, #240] */
        0x910003e1,  /* mov     x1, sp */
        0xf9007c41,  /* str     x1, [x2, #248] */
        0x90000001,  /* adrp    x1, 1f */
        0x9101e021,  /* add     x1, x1, #:lo12:1f */
        0xf9008041,  /* str     x1, [x2, #256] */
        0xd53b4201,  /* mrs     x1, nzcv */
        0xb9010841,  /* str     w1, [x2, #264] */
        0xd53b4401,  /* mrs     x1, fpcr */
        0xb9010c41,  /* str     w1, [x2, #268] */
        0xd53b4421,  /* mrs     x1, fpsr */
        0xb9011041,  /* str     w1, [x2, #272] */
        0xf9400441,  /* ldr     x1, [x2, #8] */
        0xd63f0060,  /* blr     x3 */
        0xa8c17bfd,  /* 1: ldp     x29, x30, [sp], #16 */
        0xd65f03c0,  /* ret */
     };

    memcpy( func_ptr, call_func, sizeof(call_func) );

#define COMPARE(reg) \
    ok( context.reg == expect.reg, "wrong " #reg " %p/%p\n", (void *)(ULONG64)context.reg, (void *)(ULONG64)expect.reg )

    memset( &context, 0xcc, sizeof(context) );
    memset( &expect, 0xcc, sizeof(expect) );
    func_ptr( &context, 0, &expect, pRtlCaptureContext );
    trace( "expect: x0=%p x1=%p x2=%p x3=%p x4=%p x5=%p x6=%p x7=%p x8=%p x9=%p x10=%p x11=%p x12=%p x13=%p x14=%p x15=%p x16=%p x17=%p x18=%p x19=%p x20=%p x21=%p x22=%p x23=%p x24=%p x25=%p x26=%p x27=%p x28=%p fp=%p lr=%p sp=%p pc=%p cpsr=%08x\n",
           (void *)expect.X0, (void *)expect.X1, (void *)expect.X2, (void *)expect.X3,
           (void *)expect.X4, (void *)expect.X5, (void *)expect.X6, (void *)expect.X7,
           (void *)expect.X8, (void *)expect.X9, (void *)expect.X10, (void *)expect.X11,
           (void *)expect.X12, (void *)expect.X13, (void *)expect.X14, (void *)expect.X15,
           (void *)expect.X16, (void *)expect.X17, (void *)expect.X18, (void *)expect.X19,
           (void *)expect.X20, (void *)expect.X21, (void *)expect.X22, (void *)expect.X23,
           (void *)expect.X24, (void *)expect.X25, (void *)expect.X26, (void *)expect.X27,
           (void *)expect.X28, (void *)expect.Fp, (void *)expect.Lr, (void *)expect.Sp,
           (void *)expect.Pc, expect.Cpsr );
    trace( "actual: x0=%p x1=%p x2=%p x3=%p x4=%p x5=%p x6=%p x7=%p x8=%p x9=%p x10=%p x11=%p x12=%p x13=%p x14=%p x15=%p x16=%p x17=%p x18=%p x19=%p x20=%p x21=%p x22=%p x23=%p x24=%p x25=%p x26=%p x27=%p x28=%p fp=%p lr=%p sp=%p pc=%p cpsr=%08x\n",
           (void *)context.X0, (void *)context.X1, (void *)context.X2, (void *)context.X3,
           (void *)context.X4, (void *)context.X5, (void *)context.X6, (void *)context.X7,
           (void *)context.X8, (void *)context.X9, (void *)context.X10, (void *)context.X11,
           (void *)context.X12, (void *)context.X13, (void *)context.X14, (void *)context.X15,
           (void *)context.X16, (void *)context.X17, (void *)context.X18, (void *)context.X19,
           (void *)context.X20, (void *)context.X21, (void *)context.X22, (void *)context.X23,
           (void *)context.X24, (void *)context.X25, (void *)context.X26, (void *)context.X27,
           (void *)context.X28, (void *)context.Fp, (void *)context.Lr, (void *)context.Sp,
           (void *)context.Pc, context.Cpsr );

    ok( context.ContextFlags == CONTEXT_FULL,
        "wrong flags %08x\n", context.ContextFlags );
    ok( !context.X0, "wrong X0 %p\n", (void *)context.X0 );
    COMPARE( X1 );
    COMPARE( X2 );
    COMPARE( X3 );
    COMPARE( X4 );
    COMPARE( X5 );
    COMPARE( X6 );
    COMPARE( X7 );
    COMPARE( X8 );
    COMPARE( X9 );
    COMPARE( X10 );
    COMPARE( X11 );
    COMPARE( X12 );
    COMPARE( X13 );
    COMPARE( X14 );
    COMPARE( X15 );
    COMPARE( X16 );
    COMPARE( X17 );
    COMPARE( X18 );
    COMPARE( X19 );
    COMPARE( X20 );
    COMPARE( X21 );
    COMPARE( X22 );
    COMPARE( X23 );
    COMPARE( X24 );
    COMPARE( X25 );
    COMPARE( X26 );
    COMPARE( X27 );
    COMPARE( X28 );
    COMPARE( Fp );
    COMPARE( Sp );
    COMPARE( Pc );
    COMPARE( Cpsr );
    COMPARE( Fpcr );
    COMPARE( Fpsr );
    ok( !context.Lr, "wrong Lr %p\n", (void *)context.Lr );

    memset( &context, 0xcc, sizeof(context) );
    memset( &expect, 0xcc, sizeof(expect) );
    context.ContextFlags = CONTEXT_FULL;

    status = func_ptr( GetCurrentThread(), &context, &expect, pNtGetContextThread );
    ok( status == STATUS_SUCCESS, "NtGetContextThread failed %08x\n", status );
    trace( "expect: x0=%p x1=%p x2=%p x3=%p x4=%p x5=%p x6=%p x7=%p x8=%p x9=%p x10=%p x11=%p x12=%p x13=%p x14=%p x15=%p x16=%p x17=%p x18=%p x19=%p x20=%p x21=%p x22=%p x23=%p x24=%p x25=%p x26=%p x27=%p x28=%p fp=%p lr=%p sp=%p pc=%p cpsr=%08x\n",
           (void *)expect.X0, (void *)expect.X1, (void *)expect.X2, (void *)expect.X3,
           (void *)expect.X4, (void *)expect.X5, (void *)expect.X6, (void *)expect.X7,
           (void *)expect.X8, (void *)expect.X9, (void *)expect.X10, (void *)expect.X11,
           (void *)expect.X12, (void *)expect.X13, (void *)expect.X14, (void *)expect.X15,
           (void *)expect.X16, (void *)expect.X17, (void *)expect.X18, (void *)expect.X19,
           (void *)expect.X20, (void *)expect.X21, (void *)expect.X22, (void *)expect.X23,
           (void *)expect.X24, (void *)expect.X25, (void *)expect.X26, (void *)expect.X27,
           (void *)expect.X28, (void *)expect.Fp, (void *)expect.Lr, (void *)expect.Sp,
           (void *)expect.Pc, expect.Cpsr );
    trace( "actual: x0=%p x1=%p x2=%p x3=%p x4=%p x5=%p x6=%p x7=%p x8=%p x9=%p x10=%p x11=%p x12=%p x13=%p x14=%p x15=%p x16=%p x17=%p x18=%p x19=%p x20=%p x21=%p x22=%p x23=%p x24=%p x25=%p x26=%p x27=%p x28=%p fp=%p lr=%p sp=%p pc=%p cpsr=%08x\n",
           (void *)context.X0, (void *)context.X1, (void *)context.X2, (void *)context.X3,
           (void *)context.X4, (void *)context.X5, (void *)context.X6, (void *)context.X7,
           (void *)context.X8, (void *)context.X9, (void *)context.X10, (void *)context.X11,
           (void *)context.X12, (void *)context.X13, (void *)context.X14, (void *)context.X15,
           (void *)context.X16, (void *)context.X17, (void *)context.X18, (void *)context.X19,
           (void *)context.X20, (void *)context.X21, (void *)context.X22, (void *)context.X23,
           (void *)context.X24, (void *)context.X25, (void *)context.X26, (void *)context.X27,
           (void *)context.X28, (void *)context.Fp, (void *)context.Lr, (void *)context.Sp,
           (void *)context.Pc, context.Cpsr );
    /* other registers are not preserved */
    COMPARE( X19 );
    COMPARE( X20 );
    COMPARE( X21 );
    COMPARE( X22 );
    COMPARE( X23 );
    COMPARE( X24 );
    COMPARE( X25 );
    COMPARE( X26 );
    COMPARE( X27 );
    COMPARE( X28 );
    COMPARE( Fp );
    COMPARE( Fpcr );
    COMPARE( Fpsr );
    ok( context.Lr == expect.Pc, "wrong Lr %p/%p\n", (void *)context.Lr, (void *)expect.Pc );
    ok( context.Sp == expect.Sp, "wrong Sp %p/%p\n", (void *)context.Sp, (void *)expect.Sp );
    ok( (char *)context.Pc >= (char *)pNtGetContextThread &&
        (char *)context.Pc <= (char *)pNtGetContextThread + 32,
        "wrong Pc %p/%p\n", (void *)context.Pc, pNtGetContextThread );
#undef COMPARE
}

static void test_debugger(DWORD cont_status)
{
    char cmdline[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { 0 };
    DEBUG_EVENT de;
    DWORD continuestatus;
    PVOID code_mem_address = NULL;
    NTSTATUS status;
    SIZE_T size_read;
    BOOL ret;
    int counter = 0;
    si.cb = sizeof(si);

    if(!pNtGetContextThread || !pNtSetContextThread || !pNtReadVirtualMemory || !pNtTerminateProcess)
    {
        skip("NtGetContextThread, NtSetContextThread, NtReadVirtualMemory or NtTerminateProcess not found\n");
        return;
    }

    sprintf(cmdline, "%s %s %s %p", my_argv[0], my_argv[1], "debuggee", &test_stage);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, NULL, &si, &pi);
    ok(ret, "could not create child process error: %u\n", GetLastError());
    if (!ret)
        return;

    do
    {
        continuestatus = cont_status;
        ok(WaitForDebugEvent(&de, INFINITE), "reading debug event\n");

        ret = ContinueDebugEvent(de.dwProcessId, de.dwThreadId, 0xdeadbeef);
        ok(!ret, "ContinueDebugEvent unexpectedly succeeded\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "Unexpected last error: %u\n", GetLastError());

        if (de.dwThreadId != pi.dwThreadId)
        {
            trace("event %d not coming from main thread, ignoring\n", de.dwDebugEventCode);
            ContinueDebugEvent(de.dwProcessId, de.dwThreadId, cont_status);
            continue;
        }

        if (de.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
        {
            if(de.u.CreateProcessInfo.lpBaseOfImage != NtCurrentTeb()->Peb->ImageBaseAddress)
            {
                skip("child process loaded at different address, terminating it\n");
                pNtTerminateProcess(pi.hProcess, 0);
            }
        }
        else if (de.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
        {
            CONTEXT ctx;
            int stage;

            counter++;
            status = pNtReadVirtualMemory(pi.hProcess, &code_mem, &code_mem_address,
                                          sizeof(code_mem_address), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);
            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);

            ctx.ContextFlags = CONTEXT_FULL;
            status = pNtGetContextThread(pi.hThread, &ctx);
            ok(!status, "NtGetContextThread failed with 0x%x\n", status);

            trace("exception 0x%x at %p firstchance=%d pc=%p, x0=%p\n",
                  de.u.Exception.ExceptionRecord.ExceptionCode,
                  de.u.Exception.ExceptionRecord.ExceptionAddress,
                  de.u.Exception.dwFirstChance, (char *)ctx.Pc, (char *)ctx.X0);

            if (counter > 100)
            {
                ok(FALSE, "got way too many exceptions, probably caught in an infinite loop, terminating child\n");
                pNtTerminateProcess(pi.hProcess, 1);
            }
            else if (counter < 2) /* startup breakpoint */
            {
                /* breakpoint is inside ntdll */
                void *ntdll = GetModuleHandleA( "ntdll.dll" );
                IMAGE_NT_HEADERS *nt = RtlImageNtHeader( ntdll );

                ok( (char *)ctx.Pc >= (char *)ntdll &&
                    (char *)ctx.Pc < (char *)ntdll + nt->OptionalHeader.SizeOfImage,
                    "wrong pc %p ntdll %p-%p\n", (void *)ctx.Pc, ntdll,
                    (char *)ntdll + nt->OptionalHeader.SizeOfImage );
            }
            else
            {
                if (stage == 1)
                {
                    ok((char *)ctx.Pc == (char *)code_mem_address + 0xb, "Pc at %p instead of %p\n",
                       (char *)ctx.Pc, (char *)code_mem_address + 0xb);
                    /* setting the context from debugger does not affect the context that the
                     * exception handler gets, except on w2008 */
                    ctx.Pc = (UINT_PTR)code_mem_address + 0xd;
                    ctx.X0 = 0xf00f00f1;
                    /* let the debuggee handle the exception */
                    continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == 2)
                {
                    if (de.u.Exception.dwFirstChance)
                    {
                        /* debugger gets first chance exception with unmodified ctx.Pc */
                        ok((char *)ctx.Pc == (char *)code_mem_address + 0xb, "Pc at %p instead of %p\n",
                           (char *)ctx.Pc, (char *)code_mem_address + 0xb);
                        ctx.Pc = (UINT_PTR)code_mem_address + 0xd;
                        ctx.X0 = 0xf00f00f1;
                        /* pass exception to debuggee
                         * exception will not be handled and a second chance exception will be raised */
                        continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                    }
                    else
                    {
                        /* debugger gets context after exception handler has played with it */
                        /* ctx.Pc is the same value the exception handler got */
                        if (de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
                        {
                            ok((char *)ctx.Pc == (char *)code_mem_address + 0xa,
                               "Pc at %p instead of %p\n", (char *)ctx.Pc, (char *)code_mem_address + 0xa);
                            /* need to fixup Pc for debuggee */
                            ctx.Pc += 4;
                        }
                        else ok((char *)ctx.Pc == (char *)code_mem_address + 0xb,
                                "Pc at 0x%x instead of %p\n", ctx.Pc, (char *)code_mem_address + 0xb);
                        /* here we handle exception */
                    }
                }
                else if (stage == 7 || stage == 8)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT,
                       "expected EXCEPTION_BREAKPOINT, got %08x\n", de.u.Exception.ExceptionRecord.ExceptionCode);
                    ok((char *)ctx.Pc == (char *)code_mem_address + 0x1d,
                       "expected Pc = %p, got %p\n", (char *)code_mem_address + 0x1d, (char *)ctx.Pc);
                    if (stage == 8) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == 9 || stage == 10)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT,
                       "expected EXCEPTION_BREAKPOINT, got %08x\n", de.u.Exception.ExceptionRecord.ExceptionCode);
                    ok((char *)ctx.Pc == (char *)code_mem_address + 4,
                       "expected Pc = %p, got %p\n", (char *)code_mem_address + 4, (char *)ctx.Pc);
                    if (stage == 10) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == 11 || stage == 12 || stage == 13)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_INVALID_HANDLE,
                       "unexpected exception code %08x, expected %08x\n", de.u.Exception.ExceptionRecord.ExceptionCode,
                       EXCEPTION_INVALID_HANDLE);
                    ok(de.u.Exception.ExceptionRecord.NumberParameters == 0,
                       "unexpected number of parameters %d, expected 0\n", de.u.Exception.ExceptionRecord.NumberParameters);

                    if (stage == 12|| stage == 13) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else
                    ok(FALSE, "unexpected stage %x\n", stage);

                status = pNtSetContextThread(pi.hThread, &ctx);
                ok(!status, "NtSetContextThread failed with 0x%x\n", status);
            }
        }
        else if (de.dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT)
        {
            int stage;
            char buffer[128];

            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);

            ok(!de.u.DebugString.fUnicode, "unexpected unicode debug string event\n");
            ok(de.u.DebugString.nDebugStringLength < sizeof(buffer) - 1, "buffer not large enough to hold %d bytes\n",
               de.u.DebugString.nDebugStringLength);

            memset(buffer, 0, sizeof(buffer));
            status = pNtReadVirtualMemory(pi.hProcess, de.u.DebugString.lpDebugStringData, buffer,
                                          de.u.DebugString.nDebugStringLength, &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);

            if (stage == 3 || stage == 4)
                ok(!strcmp(buffer, "Hello World"), "got unexpected debug string '%s'\n", buffer);
            else /* ignore unrelated debug strings like 'SHIMVIEW: ShimInfo(Complete)' */
                ok(strstr(buffer, "SHIMVIEW") || !strncmp(buffer, "RTL:", 4),
                   "unexpected stage %x, got debug string event '%s'\n", stage, buffer);

            if (stage == 4) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
        }
        else if (de.dwDebugEventCode == RIP_EVENT)
        {
            int stage;

            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);

            if (stage == 5 || stage == 6)
            {
                ok(de.u.RipInfo.dwError == 0x11223344, "got unexpected rip error code %08x, expected %08x\n",
                   de.u.RipInfo.dwError, 0x11223344);
                ok(de.u.RipInfo.dwType  == 0x55667788, "got unexpected rip type %08x, expected %08x\n",
                   de.u.RipInfo.dwType, 0x55667788);
            }
            else
                ok(FALSE, "unexpected stage %x\n", stage);

            if (stage == 6) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
        }

        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, continuestatus);

    } while (de.dwDebugEventCode != EXIT_PROCESS_DEBUG_EVENT);

    wait_child_process( pi.hProcess );
    ret = CloseHandle(pi.hThread);
    ok(ret, "error %u\n", GetLastError());
    ret = CloseHandle(pi.hProcess);
    ok(ret, "error %u\n", GetLastError());
}

static void test_debug_service(DWORD numexc)
{
    /* not supported */
}

#endif  /* __aarch64__ */

#if defined(__i386__) || defined(__x86_64__)

static DWORD WINAPI register_check_thread(void *arg)
{
    NTSTATUS status;
    CONTEXT ctx;

    memset(&ctx, 0, sizeof(ctx));
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

    status = pNtGetContextThread(GetCurrentThread(), &ctx);
    ok(status == STATUS_SUCCESS, "NtGetContextThread failed with %x\n", status);
    ok(!ctx.Dr0, "expected 0, got %lx\n", (DWORD_PTR)ctx.Dr0);
    ok(!ctx.Dr1, "expected 0, got %lx\n", (DWORD_PTR)ctx.Dr1);
    ok(!ctx.Dr2, "expected 0, got %lx\n", (DWORD_PTR)ctx.Dr2);
    ok(!ctx.Dr3, "expected 0, got %lx\n", (DWORD_PTR)ctx.Dr3);
    ok(!ctx.Dr6, "expected 0, got %lx\n", (DWORD_PTR)ctx.Dr6);
    ok(!ctx.Dr7, "expected 0, got %lx\n", (DWORD_PTR)ctx.Dr7);

    return 0;
}

static void test_debug_registers(void)
{
    static const struct
    {
        ULONG_PTR dr0, dr1, dr2, dr3, dr6, dr7;
    }
    tests[] =
    {
        { 0x42424240, 0, 0x126bb070, 0x0badbad0, 0, 0xffff0115 },
        { 0x42424242, 0, 0x100f0fe7, 0x0abebabe, 0, 0x115 },
    };
    NTSTATUS status;
    CONTEXT ctx;
    HANDLE thread;
    int i;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        memset(&ctx, 0, sizeof(ctx));
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        ctx.Dr0 = tests[i].dr0;
        ctx.Dr1 = tests[i].dr1;
        ctx.Dr2 = tests[i].dr2;
        ctx.Dr3 = tests[i].dr3;
        ctx.Dr6 = tests[i].dr6;
        ctx.Dr7 = tests[i].dr7;

        status = pNtSetContextThread(GetCurrentThread(), &ctx);
        ok(status == STATUS_SUCCESS, "NtSetContextThread failed with %08x\n", status);

        memset(&ctx, 0, sizeof(ctx));
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

        status = pNtGetContextThread(GetCurrentThread(), &ctx);
        ok(status == STATUS_SUCCESS, "NtGetContextThread failed with %08x\n", status);
        ok(ctx.Dr0 == tests[i].dr0, "test %d: expected %lx, got %lx\n", i, tests[i].dr0, (DWORD_PTR)ctx.Dr0);
        ok(ctx.Dr1 == tests[i].dr1, "test %d: expected %lx, got %lx\n", i, tests[i].dr1, (DWORD_PTR)ctx.Dr1);
        ok(ctx.Dr2 == tests[i].dr2, "test %d: expected %lx, got %lx\n", i, tests[i].dr2, (DWORD_PTR)ctx.Dr2);
        ok(ctx.Dr3 == tests[i].dr3, "test %d: expected %lx, got %lx\n", i, tests[i].dr3, (DWORD_PTR)ctx.Dr3);
        ok((ctx.Dr6 &  0xf00f) == tests[i].dr6, "test %d: expected %lx, got %lx\n", i, tests[i].dr6, (DWORD_PTR)ctx.Dr6);
        ok((ctx.Dr7 & ~0xdc00) == tests[i].dr7, "test %d: expected %lx, got %lx\n", i, tests[i].dr7, (DWORD_PTR)ctx.Dr7);
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    ctx.Dr0 = 0xffffffff;
    ctx.Dr1 = 0xffffffff;
    ctx.Dr2 = 0xffffffff;
    ctx.Dr3 = 0xffffffff;
    ctx.Dr6 = 0xffffffff;
    ctx.Dr7 = 0x00000400;
    status = pNtSetContextThread(GetCurrentThread(), &ctx);
    ok(status == STATUS_SUCCESS, "NtSetContextThread failed with %x\n", status);

    thread = CreateThread(NULL, 0, register_check_thread, NULL, CREATE_SUSPENDED, NULL);
    ok(thread != INVALID_HANDLE_VALUE, "CreateThread failed with %d\n", GetLastError());

    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    status = pNtGetContextThread(thread, &ctx);
    ok(status == STATUS_SUCCESS, "NtGetContextThread failed with %x\n", status);
    ok(!ctx.Dr0, "expected 0, got %lx\n", (DWORD_PTR)ctx.Dr0);
    ok(!ctx.Dr1, "expected 0, got %lx\n", (DWORD_PTR)ctx.Dr1);
    ok(!ctx.Dr2, "expected 0, got %lx\n", (DWORD_PTR)ctx.Dr2);
    ok(!ctx.Dr3, "expected 0, got %lx\n", (DWORD_PTR)ctx.Dr3);
    ok(!ctx.Dr6, "expected 0, got %lx\n", (DWORD_PTR)ctx.Dr6);
    ok(!ctx.Dr7, "expected 0, got %lx\n", (DWORD_PTR)ctx.Dr7);

    ResumeThread(thread);
    WaitForSingleObject(thread, 10000);
    CloseHandle(thread);
}

#if defined(__x86_64__)

static void test_debug_registers_wow64(void)
{
    char cmdline[] = "C:\\windows\\syswow64\\notepad.exe";
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = {0};
    WOW64_CONTEXT wow64_ctx;
    CONTEXT ctx;
    BOOL is_wow64;
    NTSTATUS ret;
    BOOL bret;

    si.cb = sizeof(si);
    bret = CreateProcessA(cmdline, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(bret, "CreateProcessA failed\n");

    bret = pIsWow64Process(pi.hProcess, &is_wow64);
    ok(bret && is_wow64, "expected Wow64 process\n");

    SuspendThread(pi.hThread);

    ZeroMemory(&ctx, sizeof(ctx));
    ctx.ContextFlags = CONTEXT_ALL;
    bret = GetThreadContext(pi.hThread, &ctx);
    ok(bret, "GetThreadContext failed\n");

    ctx.Dr0 = 0x12340000;
    ctx.Dr1 = 0x12340001;
    ctx.Dr2 = 0x12340002;
    ctx.Dr3 = 0x12340003;
    ctx.Dr7 = 0x155; /* enable all breakpoints (local) */
    bret = SetThreadContext(pi.hThread, &ctx);
    ok(bret, "SetThreadContext failed\n");

    if (bret) {
        ZeroMemory(&ctx, sizeof(ctx));
        ctx.ContextFlags = CONTEXT_ALL;
        bret = GetThreadContext(pi.hThread, &ctx);
        ok(bret, "GetThreadContext failed\n");
        if (bret)
        {
            ok(ctx.Dr0 == 0x12340000, "expected 0x12340000, got %lx\n", ctx.Dr0);
            ok(ctx.Dr1 == 0x12340001, "expected 0x12340001, got %lx\n", ctx.Dr1);
            ok(ctx.Dr2 == 0x12340002, "expected 0x12340002, got %lx\n", ctx.Dr2);
            ok(ctx.Dr3 == 0x12340003, "expected 0x12340003, got %lx\n", ctx.Dr3);
            ok(ctx.Dr7 == 0x155, "expected 0x155, got %lx\n", ctx.Dr7);
        }

        ZeroMemory(&wow64_ctx, sizeof(wow64_ctx));
        wow64_ctx.ContextFlags = WOW64_CONTEXT_ALL;
        ret = pRtlWow64GetThreadContext(pi.hThread, &wow64_ctx);
        ok(ret == STATUS_SUCCESS, "Wow64GetThreadContext failed with %lx\n", ret);
        if (ret == STATUS_SUCCESS)
        {
            ok(wow64_ctx.Dr0 == 0x12340000, "expected 0x12340000, got %lx\n", wow64_ctx.Dr0);
            ok(wow64_ctx.Dr1 == 0x12340001, "expected 0x12340001, got %lx\n", wow64_ctx.Dr1);
            ok(wow64_ctx.Dr2 == 0x12340002, "expected 0x12340002, got %lx\n", wow64_ctx.Dr2);
            ok(wow64_ctx.Dr3 == 0x12340003, "expected 0x12340003, got %lx\n", wow64_ctx.Dr3);
            ok(wow64_ctx.Dr7 == 0x155, "expected 0x155, got %lx\n", wow64_ctx.Dr7);
        }
    }

    wow64_ctx.Dr0 = 0x56780000;
    wow64_ctx.Dr1 = 0x56780001;
    wow64_ctx.Dr2 = 0x56780002;
    wow64_ctx.Dr3 = 0x56780003;
    wow64_ctx.Dr7 = 0x101; /* enable only the first breakpoint */
    ret = pRtlWow64SetThreadContext(pi.hThread, &wow64_ctx);
    ok(ret == STATUS_SUCCESS, "Wow64SetThreadContext failed with %lx\n", ret);

    ZeroMemory(&wow64_ctx, sizeof(wow64_ctx));
    wow64_ctx.ContextFlags = WOW64_CONTEXT_ALL;
    ret = pRtlWow64GetThreadContext(pi.hThread, &wow64_ctx);
    ok(ret == STATUS_SUCCESS, "Wow64GetThreadContext failed with %lx\n", ret);
    if (ret == STATUS_SUCCESS)
    {
        ok(wow64_ctx.Dr0 == 0x56780000, "expected 0x56780000, got %lx\n", wow64_ctx.Dr0);
        ok(wow64_ctx.Dr1 == 0x56780001, "expected 0x56780001, got %lx\n", wow64_ctx.Dr1);
        ok(wow64_ctx.Dr2 == 0x56780002, "expected 0x56780002, got %lx\n", wow64_ctx.Dr2);
        ok(wow64_ctx.Dr3 == 0x56780003, "expected 0x56780003, got %lx\n", wow64_ctx.Dr3);
        ok(wow64_ctx.Dr7 == 0x101, "expected 0x101, got %lx\n", wow64_ctx.Dr7);
    }

    ZeroMemory(&ctx, sizeof(ctx));
    ctx.ContextFlags = CONTEXT_ALL;
    bret = GetThreadContext(pi.hThread, &ctx);
    ok(bret, "GetThreadContext failed\n");
    if (bret)
    {
        ok(ctx.Dr0 == 0x56780000, "expected 0x56780000, got %lx\n", ctx.Dr0);
        ok(ctx.Dr1 == 0x56780001, "expected 0x56780001, got %lx\n", ctx.Dr1);
        ok(ctx.Dr2 == 0x56780002, "expected 0x56780002, got %lx\n", ctx.Dr2);
        ok(ctx.Dr3 == 0x56780003, "expected 0x56780003, got %lx\n", ctx.Dr3);
        ok(ctx.Dr7 == 0x101, "expected 0x101, got %lx\n", ctx.Dr7);
    }

    ResumeThread(pi.hThread);
    TerminateProcess(pi.hProcess, 0);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

#endif

static DWORD debug_service_exceptions;

static LONG CALLBACK debug_service_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    EXCEPTION_RECORD *rec = ExceptionInfo->ExceptionRecord;
    trace("vect. handler %08x addr:%p\n", rec->ExceptionCode, rec->ExceptionAddress);

    ok(rec->ExceptionCode == EXCEPTION_BREAKPOINT, "ExceptionCode is %08x instead of %08x\n",
       rec->ExceptionCode, EXCEPTION_BREAKPOINT);

#ifdef __i386__
    ok(ExceptionInfo->ContextRecord->Eip == (DWORD)code_mem + 0x1c,
       "expected Eip = %x, got %x\n", (DWORD)code_mem + 0x1c, ExceptionInfo->ContextRecord->Eip);
    ok(rec->NumberParameters == (is_wow64 ? 1 : 3),
       "ExceptionParameters is %d instead of %d\n", rec->NumberParameters, is_wow64 ? 1 : 3);
    ok(rec->ExceptionInformation[0] == ExceptionInfo->ContextRecord->Eax,
       "expected ExceptionInformation[0] = %x, got %lx\n",
       ExceptionInfo->ContextRecord->Eax, rec->ExceptionInformation[0]);
    if (!is_wow64)
    {
        ok(rec->ExceptionInformation[1] == 0x11111111,
           "got ExceptionInformation[1] = %lx\n", rec->ExceptionInformation[1]);
        ok(rec->ExceptionInformation[2] == 0x22222222,
           "got ExceptionInformation[2] = %lx\n", rec->ExceptionInformation[2]);
    }
#else
    ok(ExceptionInfo->ContextRecord->Rip == (DWORD_PTR)code_mem + 0x2f,
       "expected Rip = %lx, got %lx\n", (DWORD_PTR)code_mem + 0x2f, ExceptionInfo->ContextRecord->Rip);
    ok(rec->NumberParameters == 1,
       "ExceptionParameters is %d instead of 1\n", rec->NumberParameters);
    ok(rec->ExceptionInformation[0] == ExceptionInfo->ContextRecord->Rax,
       "expected ExceptionInformation[0] = %lx, got %lx\n",
       ExceptionInfo->ContextRecord->Rax, rec->ExceptionInformation[0]);
#endif

    debug_service_exceptions++;
    return (rec->ExceptionCode == EXCEPTION_BREAKPOINT) ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
}

#ifdef __i386__

static const BYTE call_debug_service_code[] = {
    0x53,                         /* pushl %ebx */
    0x57,                         /* pushl %edi */
    0x8b, 0x44, 0x24, 0x0c,       /* movl 12(%esp),%eax */
    0xb9, 0x11, 0x11, 0x11, 0x11, /* movl $0x11111111,%ecx */
    0xba, 0x22, 0x22, 0x22, 0x22, /* movl $0x22222222,%edx */
    0xbb, 0x33, 0x33, 0x33, 0x33, /* movl $0x33333333,%ebx */
    0xbf, 0x44, 0x44, 0x44, 0x44, /* movl $0x44444444,%edi */
    0xcd, 0x2d,                   /* int $0x2d */
    0xeb,                         /* jmp $+17 */
    0x0f, 0x1f, 0x00,             /* nop */
    0x31, 0xc0,                   /* xorl %eax,%eax */
    0xeb, 0x0c,                   /* jmp $+14 */
    0x90, 0x90, 0x90, 0x90,       /* nop */
    0x90, 0x90, 0x90, 0x90,
    0x90,
    0x31, 0xc0,                   /* xorl %eax,%eax */
    0x40,                         /* incl %eax */
    0x5f,                         /* popl %edi */
    0x5b,                         /* popl %ebx */
    0xc3,                         /* ret */
};

#else

static const BYTE call_debug_service_code[] = {
    0x53,                         /* push %rbx */
    0x57,                         /* push %rdi */
    0x48, 0x89, 0xc8,             /* movl %rcx,%rax */
    0x48, 0xb9, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, /* movabs $0x1111111111111111,%rcx */
    0x48, 0xba, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, /* movabs $0x2222222222222222,%rdx */
    0x48, 0xbb, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, /* movabs $0x3333333333333333,%rbx */
    0x48, 0xbf, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, /* movabs $0x4444444444444444,%rdi */
    0xcd, 0x2d,                   /* int $0x2d */
    0xeb,                         /* jmp $+17 */
    0x0f, 0x1f, 0x00,             /* nop */
    0x48, 0x31, 0xc0,             /* xor %rax,%rax */
    0xeb, 0x0e,                   /* jmp $+16 */
    0x90, 0x90, 0x90, 0x90,       /* nop */
    0x90, 0x90, 0x90, 0x90,
    0x48, 0x31, 0xc0,             /* xor %rax,%rax */
    0x48, 0xff, 0xc0,             /* inc %rax */
    0x5f,                         /* pop %rdi */
    0x5b,                         /* pop %rbx */
    0xc3,                         /* ret */
};

#endif

static void test_debug_service(DWORD numexc)
{
    DWORD (CDECL *func)(DWORD_PTR) = code_mem;
    DWORD expected_exc, expected_ret;
    void *vectored_handler;
    DWORD ret;

    /* code will return 0 if execution resumes immediately after "int $0x2d", otherwise 1 */
    memcpy(code_mem, call_debug_service_code, sizeof(call_debug_service_code));

    vectored_handler = pRtlAddVectoredExceptionHandler(TRUE, &debug_service_handler);
    ok(vectored_handler != 0, "RtlAddVectoredExceptionHandler failed\n");

    expected_exc = numexc;
    expected_ret = (numexc != 0);

    /* BREAKPOINT_BREAK */
    debug_service_exceptions = 0;
    ret = func(0);
    ok(debug_service_exceptions == expected_exc,
       "BREAKPOINT_BREAK generated %u exceptions, expected %u\n",
       debug_service_exceptions, expected_exc);
    ok(ret == expected_ret,
       "BREAKPOINT_BREAK returned %u, expected %u\n", ret, expected_ret);

    /* BREAKPOINT_PROMPT */
    debug_service_exceptions = 0;
    ret = func(2);
    ok(debug_service_exceptions == expected_exc,
       "BREAKPOINT_PROMPT generated %u exceptions, expected %u\n",
       debug_service_exceptions, expected_exc);
    ok(ret == expected_ret,
       "BREAKPOINT_PROMPT returned %u, expected %u\n", ret, expected_ret);

    /* invalid debug service */
    debug_service_exceptions = 0;
    ret = func(6);
    ok(debug_service_exceptions == expected_exc,
       "invalid debug service generated %u exceptions, expected %u\n",
       debug_service_exceptions, expected_exc);
    ok(ret == expected_ret,
      "invalid debug service returned %u, expected %u\n", ret, expected_ret);

    expected_exc = (is_wow64 ? numexc : 0);
    expected_ret = (is_wow64 && numexc);

    /* BREAKPOINT_PRINT */
    debug_service_exceptions = 0;
    ret = func(1);
    ok(debug_service_exceptions == expected_exc,
       "BREAKPOINT_PRINT generated %u exceptions, expected %u\n",
       debug_service_exceptions, expected_exc);
    ok(ret == expected_ret,
       "BREAKPOINT_PRINT returned %u, expected %u\n", ret, expected_ret);

    /* BREAKPOINT_LOAD_SYMBOLS */
    debug_service_exceptions = 0;
    ret = func(3);
    ok(debug_service_exceptions == expected_exc,
       "BREAKPOINT_LOAD_SYMBOLS generated %u exceptions, expected %u\n",
       debug_service_exceptions, expected_exc);
    ok(ret == expected_ret,
       "BREAKPOINT_LOAD_SYMBOLS returned %u, expected %u\n", ret, expected_ret);

    /* BREAKPOINT_UNLOAD_SYMBOLS */
    debug_service_exceptions = 0;
    ret = func(4);
    ok(debug_service_exceptions == expected_exc,
       "BREAKPOINT_UNLOAD_SYMBOLS generated %u exceptions, expected %u\n",
       debug_service_exceptions, expected_exc);
    ok(ret == expected_ret,
       "BREAKPOINT_UNLOAD_SYMBOLS returned %u, expected %u\n", ret, expected_ret);

    /* BREAKPOINT_COMMAND_STRING */
    debug_service_exceptions = 0;
    ret = func(5);
    ok(debug_service_exceptions == expected_exc || broken(debug_service_exceptions == numexc),
       "BREAKPOINT_COMMAND_STRING generated %u exceptions, expected %u\n",
       debug_service_exceptions, expected_exc);
    ok(ret == expected_ret || broken(ret == (numexc != 0)),
       "BREAKPOINT_COMMAND_STRING returned %u, expected %u\n", ret, expected_ret);

    pRtlRemoveVectoredExceptionHandler(vectored_handler);
}
#endif /* defined(__i386__) || defined(__x86_64__) */

static DWORD outputdebugstring_exceptions;

static LONG CALLBACK outputdebugstring_vectored_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    PEXCEPTION_RECORD rec = ExceptionInfo->ExceptionRecord;
    trace("vect. handler %08x addr:%p\n", rec->ExceptionCode, rec->ExceptionAddress);

    ok(rec->ExceptionCode == DBG_PRINTEXCEPTION_C, "ExceptionCode is %08x instead of %08x\n",
        rec->ExceptionCode, DBG_PRINTEXCEPTION_C);
    ok(rec->NumberParameters == 2, "ExceptionParameters is %d instead of 2\n", rec->NumberParameters);
    ok(rec->ExceptionInformation[0] == 12, "ExceptionInformation[0] = %d instead of 12\n", (DWORD)rec->ExceptionInformation[0]);
    ok(!strcmp((char *)rec->ExceptionInformation[1], "Hello World"),
        "ExceptionInformation[1] = '%s' instead of 'Hello World'\n", (char *)rec->ExceptionInformation[1]);

    outputdebugstring_exceptions++;
    return EXCEPTION_CONTINUE_SEARCH;
}

static void test_outputdebugstring(DWORD numexc, BOOL todo)
{
    PVOID vectored_handler;

    if (!pRtlAddVectoredExceptionHandler || !pRtlRemoveVectoredExceptionHandler)
    {
        skip("RtlAddVectoredExceptionHandler or RtlRemoveVectoredExceptionHandler not found\n");
        return;
    }

    vectored_handler = pRtlAddVectoredExceptionHandler(TRUE, &outputdebugstring_vectored_handler);
    ok(vectored_handler != 0, "RtlAddVectoredExceptionHandler failed\n");

    outputdebugstring_exceptions = 0;
    OutputDebugStringA("Hello World");

    todo_wine_if(todo)
    ok(outputdebugstring_exceptions == numexc, "OutputDebugStringA generated %d exceptions, expected %d\n",
       outputdebugstring_exceptions, numexc);

    pRtlRemoveVectoredExceptionHandler(vectored_handler);
}

static DWORD ripevent_exceptions;

static LONG CALLBACK ripevent_vectored_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    PEXCEPTION_RECORD rec = ExceptionInfo->ExceptionRecord;
    trace("vect. handler %08x addr:%p\n", rec->ExceptionCode, rec->ExceptionAddress);

    ok(rec->ExceptionCode == DBG_RIPEXCEPTION, "ExceptionCode is %08x instead of %08x\n",
       rec->ExceptionCode, DBG_RIPEXCEPTION);
    ok(rec->NumberParameters == 2, "ExceptionParameters is %d instead of 2\n", rec->NumberParameters);
    ok(rec->ExceptionInformation[0] == 0x11223344, "ExceptionInformation[0] = %08x instead of %08x\n",
       (NTSTATUS)rec->ExceptionInformation[0], 0x11223344);
    ok(rec->ExceptionInformation[1] == 0x55667788, "ExceptionInformation[1] = %08x instead of %08x\n",
       (NTSTATUS)rec->ExceptionInformation[1], 0x55667788);

    ripevent_exceptions++;
    return (rec->ExceptionCode == DBG_RIPEXCEPTION) ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
}

static void test_ripevent(DWORD numexc)
{
    EXCEPTION_RECORD record;
    PVOID vectored_handler;

    if (!pRtlAddVectoredExceptionHandler || !pRtlRemoveVectoredExceptionHandler || !pRtlRaiseException)
    {
        skip("RtlAddVectoredExceptionHandler or RtlRemoveVectoredExceptionHandler or RtlRaiseException not found\n");
        return;
    }

    vectored_handler = pRtlAddVectoredExceptionHandler(TRUE, &ripevent_vectored_handler);
    ok(vectored_handler != 0, "RtlAddVectoredExceptionHandler failed\n");

    record.ExceptionCode = DBG_RIPEXCEPTION;
    record.ExceptionFlags = 0;
    record.ExceptionRecord = NULL;
    record.ExceptionAddress = NULL;
    record.NumberParameters = 2;
    record.ExceptionInformation[0] = 0x11223344;
    record.ExceptionInformation[1] = 0x55667788;

    ripevent_exceptions = 0;
    pRtlRaiseException(&record);
    ok(ripevent_exceptions == numexc, "RtlRaiseException generated %d exceptions, expected %d\n",
       ripevent_exceptions, numexc);

    pRtlRemoveVectoredExceptionHandler(vectored_handler);
}

static DWORD breakpoint_exceptions;

static LONG CALLBACK breakpoint_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    EXCEPTION_RECORD *rec = ExceptionInfo->ExceptionRecord;
    trace("vect. handler %08x addr:%p\n", rec->ExceptionCode, rec->ExceptionAddress);

    ok(rec->ExceptionCode == EXCEPTION_BREAKPOINT, "ExceptionCode is %08x instead of %08x\n",
       rec->ExceptionCode, EXCEPTION_BREAKPOINT);

#ifdef __i386__
    ok(ExceptionInfo->ContextRecord->Eip == (DWORD)code_mem + 1,
       "expected Eip = %x, got %x\n", (DWORD)code_mem + 1, ExceptionInfo->ContextRecord->Eip);
    ok(rec->NumberParameters == (is_wow64 ? 1 : 3),
       "ExceptionParameters is %d instead of %d\n", rec->NumberParameters, is_wow64 ? 1 : 3);
    ok(rec->ExceptionInformation[0] == 0,
       "got ExceptionInformation[0] = %lx\n", rec->ExceptionInformation[0]);
    ExceptionInfo->ContextRecord->Eip = (DWORD)code_mem + 2;
#elif defined(__x86_64__)
    ok(ExceptionInfo->ContextRecord->Rip == (DWORD_PTR)code_mem + 1,
       "expected Rip = %lx, got %lx\n", (DWORD_PTR)code_mem + 1, ExceptionInfo->ContextRecord->Rip);
    ok(rec->NumberParameters == 1,
       "ExceptionParameters is %d instead of 1\n", rec->NumberParameters);
    ok(rec->ExceptionInformation[0] == 0,
       "got ExceptionInformation[0] = %lx\n", rec->ExceptionInformation[0]);
    ExceptionInfo->ContextRecord->Rip = (DWORD_PTR)code_mem + 2;
#elif defined(__arm__)
    ok(ExceptionInfo->ContextRecord->Pc == (DWORD)code_mem + 1,
       "expected pc = %x, got %x\n", (DWORD)code_mem + 1, ExceptionInfo->ContextRecord->Pc);
    ok(rec->NumberParameters == 1,
       "ExceptionParameters is %d instead of 1\n", rec->NumberParameters);
    ok(rec->ExceptionInformation[0] == 0,
       "got ExceptionInformation[0] = %lx\n", rec->ExceptionInformation[0]);
    ExceptionInfo->ContextRecord->Pc += 2;
#elif defined(__aarch64__)
    ok(ExceptionInfo->ContextRecord->Pc == (DWORD_PTR)code_mem,
       "expected pc = %lx, got %lx\n", (DWORD_PTR)code_mem, ExceptionInfo->ContextRecord->Pc);
    ok(rec->NumberParameters == 1,
       "ExceptionParameters is %d instead of 1\n", rec->NumberParameters);
    ok(rec->ExceptionInformation[0] == 0,
       "got ExceptionInformation[0] = %lx\n", rec->ExceptionInformation[0]);
    ExceptionInfo->ContextRecord->Pc += 4;
#endif

    breakpoint_exceptions++;
    return (rec->ExceptionCode == EXCEPTION_BREAKPOINT) ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
}

#if defined(__i386__) || defined(__x86_64__)
static const BYTE breakpoint_code[] = { 0xcd, 0x03, 0xc3 };   /* int $0x3; ret */
#elif defined(__arm__)
static const DWORD breakpoint_code[] = { 0xdefe, 0x4770 };  /* udf #0xfe; bx lr */
#elif defined(__aarch64__)
static const DWORD breakpoint_code[] = { 0xd43e0000, 0xd65f03c0 };  /* brk #0xf000; ret */
#endif

static void test_breakpoint(DWORD numexc)
{
    DWORD (CDECL *func)(void) = code_mem;
    void *vectored_handler;

    memcpy(code_mem, breakpoint_code, sizeof(breakpoint_code));
#ifdef __arm__
    func = (void *)((char *)code_mem + 1);  /* thumb */
#endif
    vectored_handler = pRtlAddVectoredExceptionHandler(TRUE, &breakpoint_handler);
    ok(vectored_handler != 0, "RtlAddVectoredExceptionHandler failed\n");

    breakpoint_exceptions = 0;
    func();
    ok(breakpoint_exceptions == numexc, "int $0x3 generated %u exceptions, expected %u\n",
       breakpoint_exceptions, numexc);

    pRtlRemoveVectoredExceptionHandler(vectored_handler);
}

#if defined(__i386__) || defined(__x86_64__)
static BYTE except_code_set_ymm0[] =
{
#ifdef __x86_64__
    0x48,
#endif
    0xb8,                         /* mov imm,%ax */
    0x00, 0x00, 0x00, 0x00,
#ifdef __x86_64__
    0x00, 0x00, 0x00, 0x00,
#endif

    0xc5, 0xfc, 0x10, 0x00,       /* vmovups (%ax),%ymm0 */
    0xcc,                         /* int3 */
    0xc5, 0xfc, 0x11, 0x00,       /* vmovups %ymm0,(%ax) */
    0xc3,                         /* ret  */
};

static void test_debuggee_xstate(void)
{
    void (CDECL *func)(void) = code_mem;
    unsigned int address_offset, i;
    unsigned int data[8];

    if (!pRtlGetEnabledExtendedFeatures || !pRtlGetEnabledExtendedFeatures(1 << XSTATE_AVX))
    {
        memcpy(code_mem, breakpoint_code, sizeof(breakpoint_code));
        func();
        return;
    }

    memcpy(code_mem, except_code_set_ymm0, sizeof(except_code_set_ymm0));
    address_offset = sizeof(void *) == 8 ? 2 : 1;
    *(void **)((BYTE *)code_mem + address_offset) = data;

    for (i = 0; i < ARRAY_SIZE(data); ++i)
        data[i] = i + 1;

    func();

    for (i = 0; i < 4; ++i)
        ok(data[i] == (test_stage == 14 ? i + 1 : 0x28282828),
                "Got unexpected data %#x, test_stage %u, i %u.\n", data[i], test_stage, i);

    for (     ; i < ARRAY_SIZE(data); ++i)
        ok(data[i] == (test_stage == 14 ? i + 1 : 0x48484848)
                || broken(test_stage == 15 && data[i] == i + 1) /* Win7 */,
                "Got unexpected data %#x, test_stage %u, i %u.\n", data[i], test_stage, i);
}

static BYTE except_code_segments[] =
{
    0x8c, 0xc0, /* mov %es,%eax */
    0x50,       /* push %rax */
    0x8c, 0xd8, /* mov %ds,%eax */
    0x50,       /* push %rax */
    0x8c, 0xe0, /* mov %fs,%eax */
    0x50,       /* push %rax */
    0x8c, 0xe8, /* mov %gs,%eax */
    0x50,       /* push %rax */
    0x31, 0xc0, /* xor %eax,%eax */
    0x8e, 0xc0, /* mov %eax,%es */
    0x8e, 0xd8, /* mov %eax,%ds */
    0x8e, 0xe0, /* mov %eax,%fs */
    0x8e, 0xe8, /* mov %eax,%gs */
    0xcc,       /* int3 */
    0x58,       /* pop %rax */
    0x8e, 0xe8, /* mov %eax,%gs */
    0x58,       /* pop %rax */
    0x8e, 0xe0, /* mov %eax,%fs */
    0x58,       /* pop %rax */
    0x8e, 0xd8, /* mov %eax,%ds */
    0x58,       /* pop %rax */
    0x8e, 0xc0, /* mov %eax,%es */
    0xc3,       /* retq */
};

static void test_debuggee_segments(void)
{
    void (CDECL *func)(void) = code_mem;

    memcpy( code_mem, except_code_segments, sizeof(except_code_segments));
    func();
}
#endif

static DWORD invalid_handle_exceptions;

static LONG CALLBACK invalid_handle_vectored_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    PEXCEPTION_RECORD rec = ExceptionInfo->ExceptionRecord;
    trace("vect. handler %08x addr:%p\n", rec->ExceptionCode, rec->ExceptionAddress);

    ok(rec->ExceptionCode == EXCEPTION_INVALID_HANDLE, "ExceptionCode is %08x instead of %08x\n",
       rec->ExceptionCode, EXCEPTION_INVALID_HANDLE);
    ok(rec->NumberParameters == 0, "ExceptionParameters is %d instead of 0\n", rec->NumberParameters);

    invalid_handle_exceptions++;
    return (rec->ExceptionCode == EXCEPTION_INVALID_HANDLE) ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
}

static void test_closehandle(DWORD numexc, HANDLE handle)
{
    PVOID vectored_handler;

    if (!pRtlAddVectoredExceptionHandler || !pRtlRemoveVectoredExceptionHandler || !pRtlRaiseException)
    {
        skip("RtlAddVectoredExceptionHandler or RtlRemoveVectoredExceptionHandler or RtlRaiseException not found\n");
        return;
    }

    vectored_handler = pRtlAddVectoredExceptionHandler(TRUE, &invalid_handle_vectored_handler);
    ok(vectored_handler != 0, "RtlAddVectoredExceptionHandler failed\n");

    invalid_handle_exceptions = 0;
    CloseHandle(handle);
    ok(invalid_handle_exceptions == numexc, "CloseHandle generated %d exceptions, expected %d\n",
       invalid_handle_exceptions, numexc);

    invalid_handle_exceptions = 0;
    pNtClose(handle);
    ok(invalid_handle_exceptions == numexc, "NtClose generated %d exceptions, expected %d\n",
       invalid_handle_exceptions, numexc);

    pRtlRemoveVectoredExceptionHandler(vectored_handler);
}

static void test_vectored_continue_handler(void)
{
    PVOID handler1, handler2;
    ULONG ret;

    if (!pRtlAddVectoredContinueHandler || !pRtlRemoveVectoredContinueHandler)
    {
        skip("RtlAddVectoredContinueHandler or RtlRemoveVectoredContinueHandler not found\n");
        return;
    }

    handler1 = pRtlAddVectoredContinueHandler(TRUE, (void *)0xdeadbeef);
    ok(handler1 != 0, "RtlAddVectoredContinueHandler failed\n");

    handler2 = pRtlAddVectoredContinueHandler(TRUE, (void *)0xdeadbeef);
    ok(handler2 != 0, "RtlAddVectoredContinueHandler failed\n");
    ok(handler1 != handler2, "RtlAddVectoredContinueHandler returned same handler\n");

    if (pRtlRemoveVectoredExceptionHandler)
    {
        ret = pRtlRemoveVectoredExceptionHandler(handler1);
        ok(!ret, "RtlRemoveVectoredExceptionHandler succeeded\n");
    }

    ret = pRtlRemoveVectoredContinueHandler(handler1);
    ok(ret, "RtlRemoveVectoredContinueHandler failed\n");

    ret = pRtlRemoveVectoredContinueHandler(handler2);
    ok(ret, "RtlRemoveVectoredContinueHandler failed\n");

    ret = pRtlRemoveVectoredContinueHandler(handler1);
    ok(!ret, "RtlRemoveVectoredContinueHandler succeeded\n");

    ret = pRtlRemoveVectoredContinueHandler((void *)0x11223344);
    ok(!ret, "RtlRemoveVectoredContinueHandler succeeded\n");
}

static BOOL test_apc_called;

static void CALLBACK test_apc(ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3)
{
    test_apc_called = TRUE;
}

static void test_user_apc(void)
{
    NTSTATUS status;
    CONTEXT context;
    int pass;

    if (!pNtQueueApcThread)
    {
        win_skip("NtQueueApcThread is not available.\n");
        return;
    }

    pass = 0;
    InterlockedIncrement(&pass);
    RtlCaptureContext(&context);
    InterlockedIncrement(&pass);

    if (pass == 2)
    {
        /* Try to make sure context data is far enough below context.Esp. */
        CONTEXT c[4];

        c[0] = context;

        test_apc_called = FALSE;
        status = pNtQueueApcThread(GetCurrentThread(), test_apc, 0, 0, 0);
        ok(!status, "Got unexpected status %#x.\n", status);
        SleepEx(0, TRUE);
        ok(test_apc_called, "Test user APC was not called.\n");
        test_apc_called = FALSE;
        status = pNtQueueApcThread(GetCurrentThread(), test_apc, 0, 0, 0);
        ok(!status, "Got unexpected status %#x.\n", status);
        status = NtContinue(&c[0], TRUE );

        /* Broken before Win7, in that case NtContinue returns here instead of restoring context after calling APC. */
        ok(broken(TRUE), "Should not get here, status %#x.\n", status);
        return;
    }
    ok(pass == 3, "Got unexpected pass %d.\n", pass);
    ok(test_apc_called, "Test user APC was not called.\n");
}

static DWORD WINAPI suspend_thread_test( void *arg )
{
    HANDLE event = arg;
    WaitForSingleObject(event, INFINITE);
    return 0;
}

static void test_suspend_count(HANDLE hthread, ULONG expected_count, int line)
{
    static BOOL supported = TRUE;
    NTSTATUS status;
    ULONG count;

    if (!supported)
        return;

    count = ~0u;
    status = pNtQueryInformationThread(hthread, ThreadSuspendCount, &count, sizeof(count), NULL);
    if (status)
    {
        win_skip("ThreadSuspendCount is not supported.\n");
        supported = FALSE;
        return;
    }

    ok_(__FILE__, line)(!status, "Failed to get suspend count, status %#x.\n", status);
    ok_(__FILE__, line)(count == expected_count, "Unexpected suspend count %u.\n", count);
}

static void test_suspend_thread(void)
{
#define TEST_SUSPEND_COUNT(thread, count) test_suspend_count((thread), (count), __LINE__)
    HANDLE thread, event;
    ULONG count, len;
    NTSTATUS status;
    DWORD ret;

    status = NtSuspendThread(0, NULL);
    ok(status == STATUS_INVALID_HANDLE, "Unexpected return value %#x.\n", status);

    status = NtResumeThread(0, NULL);
    ok(status == STATUS_INVALID_HANDLE, "Unexpected return value %#x.\n", status);

    event = CreateEventW(NULL, FALSE, FALSE, NULL);

    thread = CreateThread(NULL, 0, suspend_thread_test, event, 0, NULL);
    ok(thread != NULL, "Failed to create a thread.\n");

    ret = WaitForSingleObject(thread, 0);
    ok(ret == WAIT_TIMEOUT, "Unexpected status %d.\n", ret);

    status = pNtQueryInformationThread(thread, ThreadSuspendCount, &count, sizeof(count), NULL);
    if (!status)
    {
        status = pNtQueryInformationThread(thread, ThreadSuspendCount, NULL, sizeof(count), NULL);
        ok(status == STATUS_ACCESS_VIOLATION, "Unexpected status %#x.\n", status);

        status = pNtQueryInformationThread(thread, ThreadSuspendCount, &count, sizeof(count) / 2, NULL);
        ok(status == STATUS_INFO_LENGTH_MISMATCH, "Unexpected status %#x.\n", status);

        len = 123;
        status = pNtQueryInformationThread(thread, ThreadSuspendCount, &count, sizeof(count) / 2, &len);
        ok(status == STATUS_INFO_LENGTH_MISMATCH, "Unexpected status %#x.\n", status);
        ok(len == 123, "Unexpected info length %u.\n", len);

        len = 123;
        status = pNtQueryInformationThread(thread, ThreadSuspendCount, NULL, 0, &len);
        ok(status == STATUS_INFO_LENGTH_MISMATCH, "Unexpected status %#x.\n", status);
        ok(len == 123, "Unexpected info length %u.\n", len);

        count = 10;
        status = pNtQueryInformationThread(0, ThreadSuspendCount, &count, sizeof(count), NULL);
        ok(status, "Unexpected status %#x.\n", status);
        ok(count == 10, "Unexpected suspend count %u.\n", count);
    }

    status = NtResumeThread(thread, NULL);
    ok(!status, "Unexpected status %#x.\n", status);

    status = NtResumeThread(thread, &count);
    ok(!status, "Unexpected status %#x.\n", status);
    ok(count == 0, "Unexpected suspended count %u.\n", count);

    TEST_SUSPEND_COUNT(thread, 0);

    status = NtSuspendThread(thread, NULL);
    ok(!status, "Failed to suspend a thread, status %#x.\n", status);

    TEST_SUSPEND_COUNT(thread, 1);

    status = NtSuspendThread(thread, &count);
    ok(!status, "Failed to suspend a thread, status %#x.\n", status);
    ok(count == 1, "Unexpected suspended count %u.\n", count);

    TEST_SUSPEND_COUNT(thread, 2);

    status = NtResumeThread(thread, &count);
    ok(!status, "Failed to resume a thread, status %#x.\n", status);
    ok(count == 2, "Unexpected suspended count %u.\n", count);

    TEST_SUSPEND_COUNT(thread, 1);

    status = NtResumeThread(thread, NULL);
    ok(!status, "Failed to resume a thread, status %#x.\n", status);

    TEST_SUSPEND_COUNT(thread, 0);

    SetEvent(event);
    WaitForSingleObject(thread, INFINITE);

    CloseHandle(thread);
#undef TEST_SUSPEND_COUNT
}

static const char *suspend_process_event_name = "suspend_process_event";
static const char *suspend_process_event2_name = "suspend_process_event2";

static DWORD WINAPI dummy_thread_proc( void *arg )
{
    return 0;
}

static void suspend_process_proc(void)
{
    HANDLE event = OpenEventA(SYNCHRONIZE, FALSE, suspend_process_event_name);
    HANDLE event2 = OpenEventA(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, suspend_process_event2_name);
    unsigned int count;
    NTSTATUS status;
    HANDLE thread;

    ok(event != NULL, "Failed to open event handle.\n");
    ok(event2 != NULL, "Failed to open event handle.\n");

    thread = CreateThread(NULL, 0, dummy_thread_proc, 0, CREATE_SUSPENDED, NULL);
    ok(thread != NULL, "Failed to create auxiliary thread.\n");

    /* Suspend up to limit. */
    while (!(status = NtSuspendThread(thread, NULL)))
        ;
    ok(status == STATUS_SUSPEND_COUNT_EXCEEDED, "Unexpected status %#x.\n", status);

    for (;;)
    {
        SetEvent(event2);
        if (WaitForSingleObject(event, 100) == WAIT_OBJECT_0)
            break;
    }

    status = NtSuspendThread(thread, &count);
    ok(!status, "Failed to suspend a thread, status %#x.\n", status);
    ok(count == 125, "Unexpected suspend count %u.\n", count);

    status = NtResumeThread(thread, NULL);
    ok(!status, "Failed to resume a thread, status %#x.\n", status);

    CloseHandle(event);
    CloseHandle(event2);
}

static void test_suspend_process(void)
{
    PROCESS_INFORMATION info;
    char path_name[MAX_PATH];
    STARTUPINFOA startup;
    HANDLE event, event2;
    NTSTATUS status;
    char **argv;
    DWORD ret;

    event = CreateEventA(NULL, FALSE, FALSE, suspend_process_event_name);
    ok(event != NULL, "Failed to create event.\n");

    event2 = CreateEventA(NULL, FALSE, FALSE, suspend_process_event2_name);
    ok(event2 != NULL, "Failed to create event.\n");

    winetest_get_mainargs(&argv);
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    sprintf(path_name, "%s exception suspend_process", argv[0]);

    ret = CreateProcessA(NULL, path_name, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info);
    ok(ret, "Failed to create target process.\n");

    /* New process signals this event. */
    ResetEvent(event2);
    ret = WaitForSingleObject(event2, INFINITE);
    ok(ret == WAIT_OBJECT_0, "Wait failed, %#x.\n", ret);

    /* Suspend main thread */
    status = NtSuspendThread(info.hThread, &ret);
    ok(!status && !ret, "Failed to suspend main thread, status %#x.\n", status);

    /* Process wasn't suspended yet. */
    status = pNtResumeProcess(info.hProcess);
    ok(!status, "Failed to resume a process, status %#x.\n", status);

    status = pNtSuspendProcess(0);
    ok(status == STATUS_INVALID_HANDLE, "Unexpected status %#x.\n", status);

    status = pNtResumeProcess(info.hProcess);
    ok(!status, "Failed to resume a process, status %#x.\n", status);

    ResetEvent(event2);
    ret = WaitForSingleObject(event2, 200);
    ok(ret == WAIT_OBJECT_0, "Wait failed.\n");

    status = pNtSuspendProcess(info.hProcess);
    ok(!status, "Failed to suspend a process, status %#x.\n", status);

    status = NtSuspendThread(info.hThread, &ret);
    ok(!status && ret == 1, "Failed to suspend main thread, status %#x.\n", status);
    status = NtResumeThread(info.hThread, &ret);
    ok(!status && ret == 2, "Failed to resume main thread, status %#x.\n", status);

    ResetEvent(event2);
    ret = WaitForSingleObject(event2, 200);
    ok(ret == WAIT_TIMEOUT, "Wait failed.\n");

    status = pNtSuspendProcess(info.hProcess);
    ok(!status, "Failed to suspend a process, status %#x.\n", status);

    status = pNtResumeProcess(info.hProcess);
    ok(!status, "Failed to resume a process, status %#x.\n", status);

    ResetEvent(event2);
    ret = WaitForSingleObject(event2, 200);
    ok(ret == WAIT_TIMEOUT, "Wait failed.\n");

    status = pNtResumeProcess(info.hProcess);
    ok(!status, "Failed to resume a process, status %#x.\n", status);

    ResetEvent(event2);
    ret = WaitForSingleObject(event2, 200);
    ok(ret == WAIT_OBJECT_0, "Wait failed.\n");

    SetEvent(event);

    wait_child_process(info.hProcess);

    CloseHandle(info.hProcess);
    CloseHandle(info.hThread);

    CloseHandle(event);
    CloseHandle(event2);
}

static void test_unload_trace(void)
{
    static const WCHAR imageW[] = {'m','s','x','m','l','3','.','d','l','l',0};
    RTL_UNLOAD_EVENT_TRACE *unload_trace, **unload_trace_ex = NULL, *ptr;
    ULONG *element_size, *element_count, size;
    HMODULE hmod;
    BOOL found;

    unload_trace = pRtlGetUnloadEventTrace();
    ok(unload_trace != NULL, "Failed to get unload events pointer.\n");

    if (pRtlGetUnloadEventTraceEx)
    {
        pRtlGetUnloadEventTraceEx(&element_size, &element_count, (void **)&unload_trace_ex);
        ok(*element_size >= sizeof(*ptr), "Unexpected element size.\n");
        ok(*element_count == RTL_UNLOAD_EVENT_TRACE_NUMBER, "Unexpected trace element count %u.\n", *element_count);
        ok(unload_trace_ex != NULL, "Unexpected pointer %p.\n", unload_trace_ex);
        size = *element_size;
    }
    else
        size = sizeof(*unload_trace);

    hmod = LoadLibraryA("msxml3.dll");
    ok(hmod != NULL, "Failed to load library.\n");
    FreeLibrary(hmod);

    found = FALSE;
    ptr = unload_trace;
    while (ptr->BaseAddress != NULL)
    {
        if (!lstrcmpW(imageW, ptr->ImageName))
        {
            found = TRUE;
            break;
        }
        ptr = (RTL_UNLOAD_EVENT_TRACE *)((char *)ptr + size);
    }
    ok(found, "Unloaded module wasn't found.\n");

    if (unload_trace_ex)
    {
        found = FALSE;
        ptr = *unload_trace_ex;
        while (ptr->BaseAddress != NULL)
        {
            if (!lstrcmpW(imageW, ptr->ImageName))
            {
                found = TRUE;
                break;
            }
            ptr = (RTL_UNLOAD_EVENT_TRACE *)((char *)ptr + size);
        }
        ok(found, "Unloaded module wasn't found.\n");
    }
}

#if defined(__i386__) || defined(__x86_64__)

static const unsigned int test_extended_context_data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
static const unsigned test_extended_context_spoil_data1[8] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80};
static const unsigned test_extended_context_spoil_data2[8] = {0x15, 0x25, 0x35, 0x45, 0x55, 0x65, 0x75, 0x85};

static BOOL test_extended_context_modified_state;
static BOOL xsaveopt_enabled, compaction_enabled;

static DWORD test_extended_context_handler(EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
        CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher)
{
    static const ULONG64 expected_compaction_mask = 0x8000000000000004;
    CONTEXT_EX *xctx = (CONTEXT_EX *)(context + 1);
    unsigned int *context_ymm_data;
    DWORD expected_min_offset;
    XSTATE *xs;

    ok((context->ContextFlags & (CONTEXT_FULL | CONTEXT_XSTATE)) == (CONTEXT_FULL | CONTEXT_XSTATE),
            "Got unexpected ContextFlags %#x.\n", context->ContextFlags);

    if ((context->ContextFlags & (CONTEXT_FULL | CONTEXT_XSTATE)) != (CONTEXT_FULL | CONTEXT_XSTATE))
        goto done;

#ifdef __x86_64__
    {
        /* Unwind contexts do not inherit xstate information. */
        DISPATCHER_CONTEXT *dispatch = (DISPATCHER_CONTEXT *)dispatcher;

        ok(!(dispatch->ContextRecord->ContextFlags & 0x40), "Got unexpected ContextRecord->ContextFlags %#x.\n",
                dispatch->ContextRecord->ContextFlags);
    }
#endif

    ok(xctx->Legacy.Offset == -(int)(sizeof(CONTEXT)), "Got unexpected Legacy.Offset %d.\n", xctx->Legacy.Offset);
    ok(xctx->Legacy.Length == sizeof(CONTEXT), "Got unexpected Legacy.Length %d.\n", xctx->Legacy.Length);
    ok(xctx->All.Offset == -(int)sizeof(CONTEXT), "Got unexpected All.Offset %d.\n", xctx->All.Offset);
    ok(xctx->All.Length == sizeof(CONTEXT) + xctx->XState.Offset + xctx->XState.Length,
            "Got unexpected All.Offset %d.\n", xctx->All.Offset);
    expected_min_offset = sizeof(void *) == 8 ? sizeof(CONTEXT_EX) + sizeof(EXCEPTION_RECORD) : sizeof(CONTEXT_EX);
    ok(xctx->XState.Offset >= expected_min_offset,
            "Got unexpected XState.Offset %d.\n", xctx->XState.Offset);
    ok(xctx->XState.Length >= sizeof(XSTATE), "Got unexpected XState.Length %d.\n", xctx->XState.Length);

    xs = (XSTATE *)((char *)xctx + xctx->XState.Offset);
    context_ymm_data = (unsigned int *)&xs->YmmContext;
    ok(!((ULONG_PTR)xs % 64), "Got unexpected xs %p.\n", xs);

    if (compaction_enabled)
        ok((xs->CompactionMask & (expected_compaction_mask | 3)) == expected_compaction_mask,
                "Got compaction mask %#I64x.\n", xs->CompactionMask);
    else
        ok(!xs->CompactionMask, "Got compaction mask %#I64x.\n", xs->CompactionMask);

    if (test_extended_context_modified_state)
    {
        ok((xs->Mask & 7) == 4, "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
        ok(!memcmp(context_ymm_data, test_extended_context_data + 4, sizeof(M128A)),
                "Got unexpected context data.\n");
    }
    else
    {
        ok(xs->Mask == (xsaveopt_enabled ? 0 : 4), "Got unexpected Mask %#I64x.\n", xs->Mask);
        /* The save area has garbage if xsaveopt is available, so we can't test
         * its contents. */

        /* Clear the mask; the state should be restored to INIT_STATE without
         * using this data. */
        xs->Mask = 0;
        memcpy(context_ymm_data, test_extended_context_spoil_data1 + 4, sizeof(M128A));
    }

done:
#ifdef __GNUC__
    __asm__ volatile("vmovups %0,%%ymm0" : : "m"(test_extended_context_spoil_data2));
#endif
#ifdef __x86_64__
    ++context->Rip;
#else
    if (*(BYTE *)context->Eip == 0xcc)
        ++context->Eip;
#endif
    return ExceptionContinueExecution;
}

struct call_func_offsets
{
    unsigned int func_addr;
    unsigned int func_param1;
    unsigned int func_param2;
    unsigned int ymm0_save;
};
#ifdef __x86_64__
static BYTE call_func_code_set_ymm0[] =
{
    0x55,                         /* pushq %rbp */
    0x48, 0xb8,                   /* mov imm,%rax */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x48, 0xb9,                   /* mov imm,%rcx */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x48, 0xba,                   /* mov imm,%rdx */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x48, 0xbd,                   /* mov imm,%rbp */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0xc5, 0xfc, 0x10, 0x45, 0x00, /* vmovups (%rbp),%ymm0 */
    0x48, 0x83, 0xec, 0x20,       /* sub $0x20,%rsp */
    0xff, 0xd0,                   /* call *rax */
    0x48, 0x83, 0xc4, 0x20,       /* add $0x20,%rsp */
    0xc5, 0xfc, 0x11, 0x45, 0x00, /* vmovups %ymm0,(%rbp) */
    0x5d,                         /* popq %rbp */
    0xc3,                         /* ret  */
};
static BYTE call_func_code_reset_ymm_state[] =
{
    0x55,                         /* pushq %rbp */
    0x48, 0xb8,                   /* mov imm,%rax */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x48, 0xb9,                   /* mov imm,%rcx */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x48, 0xba,                   /* mov imm,%rdx */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x48, 0xbd,                   /* mov imm,%rbp */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0xc5, 0xf8, 0x77,             /* vzeroupper */
    0x0f, 0x57, 0xc0,             /* xorps  %xmm0,%xmm0 */
    0x48, 0x83, 0xec, 0x20,       /* sub $0x20,%rsp */
    0xff, 0xd0,                   /* call *rax */
    0x48, 0x83, 0xc4, 0x20,       /* add $0x20,%rsp */
    0xc5, 0xfc, 0x11, 0x45, 0x00, /* vmovups %ymm0,(%rbp) */
    0x5d,                         /* popq %rbp */
    0xc3,                         /* ret  */
};
static const struct call_func_offsets call_func_offsets = {3, 13, 23, 33};
#else
static BYTE call_func_code_set_ymm0[] =
{
    0x55,                         /* pushl %ebp */
    0xb8,                         /* mov imm,%eax */
    0x00, 0x00, 0x00, 0x00,

    0xb9,                         /* mov imm,%ecx */
    0x00, 0x00, 0x00, 0x00,

    0xba,                         /* mov imm,%edx */
    0x00, 0x00, 0x00, 0x00,

    0xbd,                         /* mov imm,%ebp */
    0x00, 0x00, 0x00, 0x00,

    0x81, 0xfa, 0xef, 0xbe, 0xad, 0xde,
                                  /* cmpl $0xdeadbeef, %edx */
    0x74, 0x01,                   /* je 1f */
    0x52,                         /* pushl %edx */
    0x51,                         /* 1: pushl %ecx */
    0xc5, 0xfc, 0x10, 0x45, 0x00, /* vmovups (%ebp),%ymm0 */
    0xff, 0xd0,                   /* call *eax */
    0xc5, 0xfc, 0x11, 0x45, 0x00, /* vmovups %ymm0,(%ebp) */
    0x5d,                         /* popl %ebp */
    0xc3,                         /* ret  */
};
static BYTE call_func_code_reset_ymm_state[] =
{
    0x55,                         /* pushl %ebp */
    0xb8,                         /* mov imm,%eax */
    0x00, 0x00, 0x00, 0x00,

    0xb9,                         /* mov imm,%ecx */
    0x00, 0x00, 0x00, 0x00,

    0xba,                         /* mov imm,%edx */
    0x00, 0x00, 0x00, 0x00,

    0xbd,                         /* mov imm,%ebp */
    0x00, 0x00, 0x00, 0x00,

    0x81, 0xfa, 0xef, 0xbe, 0xad, 0xde,
                                  /* cmpl $0xdeadbeef, %edx */
    0x74, 0x01,                   /* je 1f */
    0x52,                         /* pushl %edx */
    0x51,                         /* 1: pushl %ecx */
    0xc5, 0xf8, 0x77,             /* vzeroupper */
    0x0f, 0x57, 0xc0,             /* xorps  %xmm0,%xmm0 */
    0xff, 0xd0,                   /* call *eax */
    0xc5, 0xfc, 0x11, 0x45, 0x00, /* vmovups %ymm0,(%ebp) */
    0x5d,                         /* popl %ebp */
    0xc3,                         /* ret  */
};
static const struct call_func_offsets call_func_offsets = {2, 7, 12, 17};
#endif

static DWORD WINAPI test_extended_context_thread(void *arg)
{
    ULONG (WINAPI* func)(void) = code_mem;
    static unsigned int data[8];
    unsigned int i;

    memcpy(code_mem, call_func_code_reset_ymm_state, sizeof(call_func_code_reset_ymm_state));
    *(void **)((BYTE *)code_mem + call_func_offsets.func_addr) = SuspendThread;
    *(void **)((BYTE *)code_mem + call_func_offsets.func_param1) = (void *)GetCurrentThread();
    *(void **)((BYTE *)code_mem + call_func_offsets.func_param2) = (void *)0xdeadbeef;
    *(void **)((BYTE *)code_mem + call_func_offsets.ymm0_save) = data;
    func();

    for (i = 0; i < 4; ++i)
        ok(!data[i], "Got unexpected data %#x, i %u.\n", data[i], i);
    for (; i < 8; ++i)
        ok(data[i] == 0x48484848, "Got unexpected data %#x, i %u.\n", data[i], i);
    memset(data, 0x68, sizeof(data));

    memcpy(code_mem, call_func_code_set_ymm0, sizeof(call_func_code_set_ymm0));
    *(void **)((BYTE *)code_mem + call_func_offsets.func_addr) = SuspendThread;
    *(void **)((BYTE *)code_mem + call_func_offsets.func_param1) = (void *)GetCurrentThread();
    *(void **)((BYTE *)code_mem + call_func_offsets.func_param2) = (void *)0xdeadbeef;
    *(void **)((BYTE *)code_mem + call_func_offsets.ymm0_save) = data;
    func();

    memcpy(code_mem, call_func_code_reset_ymm_state, sizeof(call_func_code_reset_ymm_state));
    *(void **)((BYTE *)code_mem + call_func_offsets.func_addr) = SuspendThread;
    *(void **)((BYTE *)code_mem + call_func_offsets.func_param1) = (void *)GetCurrentThread();
    *(void **)((BYTE *)code_mem + call_func_offsets.func_param2) = (void *)0xdeadbeef;
    *(void **)((BYTE *)code_mem + call_func_offsets.ymm0_save) = data;
    func();
    return 0;
}

static void wait_for_thread_next_suspend(HANDLE thread)
{
    DWORD result;

    result = ResumeThread(thread);
    ok(result == 1, "Got unexpexted suspend count %u.\n", result);

    /* NtQueryInformationThread(ThreadSuspendCount, ...) is not supported on older Windows. */
    while (!(result = SuspendThread(thread)))
    {
        ResumeThread(thread);
        Sleep(1);
    }
    ok(result == 1, "Got unexpexted suspend count %u.\n", result);
    result = ResumeThread(thread);
    ok(result == 2, "Got unexpexted suspend count %u.\n", result);
}

#define CONTEXT_NATIVE (CONTEXT_XSTATE & CONTEXT_CONTROL)

static void test_extended_context(void)
{
    static BYTE except_code_reset_ymm_state[] =
    {
#ifdef __x86_64__
        0x48,
#endif
        0xb8,                         /* mov imm,%ax */
        0x00, 0x00, 0x00, 0x00,
#ifdef __x86_64__
        0x00, 0x00, 0x00, 0x00,
#endif
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

        0xc5, 0xf8, 0x77,             /* vzeroupper */
        0x0f, 0x57, 0xc0,             /* xorps  %xmm0,%xmm0 */
        0xcc,                         /* int3 */
        0xc5, 0xfc, 0x11, 0x00,       /* vmovups %ymm0,(%ax) */
        0xc3,                         /* ret  */
    };

    static const struct
    {
        ULONG flag;
        ULONG supported_flags;
        ULONG broken_flags;
        ULONG context_length;
        ULONG legacy_length;
        ULONG context_ex_length;
        ULONG align;
        ULONG flags_offset;
        ULONG xsavearea_offset;
        ULONG vector_reg_count;
    }
    context_arch[] =
    {
        {
            0x00100000,  /* CONTEXT_AMD64 */
            0xd800005f,
            0xd8000000,
            0x4d0,       /* sizeof(CONTEXT) */
            0x4d0,       /* sizeof(CONTEXT) */
            0x20,        /* sizeof(CONTEXT_EX) */
            7,
            0x30,
            0x100,       /* offsetof(CONTEXT, FltSave) */
            16,
        },
        {
            0x00010000,  /* CONTEXT_X86  */
            0xd800007f,
            0xd8000000,
            0x2cc,       /* sizeof(CONTEXT) */
            0xcc,        /* offsetof(CONTEXT, ExtendedRegisters) */
            0x18,        /* sizeof(CONTEXT_EX) */
            3,
            0,
            0xcc,        /* offsetof(CONTEXT, ExtendedRegisters) */
            8,
        },
    };
    ULONG expected_length, expected_length_xstate, context_flags, expected_offset;
    ULONG64 enabled_features, expected_compaction;
    DECLSPEC_ALIGN(64) BYTE context_buffer2[2048];
    DECLSPEC_ALIGN(64) BYTE context_buffer[2048];
    unsigned int i, j, address_offset, test;
    ULONG ret, ret2, length, length2, align;
    ULONG flags, flags_fpx, expected_flags;
    ULONG (WINAPI* func)(void) = code_mem;
    CONTEXT_EX *context_ex;
    CONTEXT *context;
    unsigned data[8];
    HANDLE thread;
    ULONG64 mask;
    XSTATE *xs;
    BOOL bret;
    void *p;

    address_offset = sizeof(void *) == 8 ? 2 : 1;
    *(void **)(except_code_set_ymm0 + address_offset) = data;
    *(void **)(except_code_reset_ymm_state + address_offset) = data;

    if (!pRtlGetEnabledExtendedFeatures)
    {
        skip("RtlGetEnabledExtendedFeatures is not available.\n");
        return;
    }

    enabled_features = pRtlGetEnabledExtendedFeatures(~(ULONG64)0);

    if (enabled_features)
    {
        int regs[4];

        __cpuidex(regs, 0xd, 1);
        xsaveopt_enabled = regs[0] & 1;
        compaction_enabled = regs[0] & 2;
    }

    /* Test context manipulation functions. */
    length = 0xdeadbeef;
    ret = pRtlGetExtendedContextLength(0, &length);
    ok(ret == STATUS_INVALID_PARAMETER && length == 0xdeadbeef, "Got unexpected result ret %#x, length %#x.\n",
            ret, length);

    for (test = 0; test < ARRAY_SIZE(context_arch); ++test)
    {
        expected_length = context_arch[test].context_length + context_arch[test].context_ex_length
                + context_arch[test].align;
        expected_length_xstate = context_arch[test].context_length + context_arch[test].context_ex_length
                + sizeof(XSTATE) + 63;

        length = 0xdeadbeef;
        ret = pRtlGetExtendedContextLength(context_arch[test].flag, &length);
        ok(!ret && length == expected_length, "Got unexpected result ret %#x, length %#x.\n",
                ret, length);

        for (i = 0; i < 32; ++i)
        {
            if (i == 6) /* CONTEXT_XSTATE */
                continue;

            flags = context_arch[test].flag | (1 << i);
            length = length2 = 0xdeadbeef;
            ret = pRtlGetExtendedContextLength(flags, &length);

            if ((context_arch[test].supported_flags & flags) || flags == context_arch[test].flag)
            {
                ok((!ret && length == expected_length)
                        || broken((context_arch[test].broken_flags & (1 << i))
                        && ret == STATUS_INVALID_PARAMETER && length == 0xdeadbeef),
                        "Got unexpected result ret %#x, length %#x, flags 0x%08x.\n",
                        ret, length, flags);
            }
            else
            {
                ok(ret == STATUS_INVALID_PARAMETER && length == 0xdeadbeef,
                        "Got unexpected result ret %#x, length %#x, flags 0x%08x.\n", ret, length, flags);
            }

            SetLastError(0xdeadbeef);
            bret = pInitializeContext(NULL, flags, NULL, &length2);
            ok(!bret && length2 == length && GetLastError()
                    == (!ret ? ERROR_INSUFFICIENT_BUFFER : ERROR_INVALID_PARAMETER),
                    "Got unexpected bret %#x, length2 %#x, GetLastError() %u, flags %#x.\n",
                    bret, length2, GetLastError(), flags);

            if (GetLastError() == ERROR_INVALID_PARAMETER)
                continue;

            SetLastError(0xdeadbeef);
            context = (void *)0xdeadbeef;
            length2 = expected_length - 1;
            bret = pInitializeContext(context_buffer, flags, &context, &length2);
            ok(!bret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
                    "Got unexpected bret %#x, GetLastError() %u, flags %#x.\n", bret, GetLastError(), flags);
            ok(context == (void *)0xdeadbeef, "Got unexpected context %p.\n", context);

            SetLastError(0xdeadbeef);
            memset(context_buffer, 0xcc, sizeof(context_buffer));
            length2 = expected_length;
            bret = pInitializeContext(context_buffer, flags, &context, &length2);
            ok(bret && GetLastError() == 0xdeadbeef,
                    "Got unexpected bret %#x, GetLastError() %u, flags %#x.\n", bret, GetLastError(), flags);
            ok(length2 == expected_length, "Got unexpexted length %#x.\n", length);
            ok((BYTE *)context == context_buffer, "Got unexpected context %p, flags %#x.\n", context, flags);

            context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
            ok(context_flags == flags, "Got unexpected ContextFlags %#x, flags %#x.\n", context_flags, flags);

            context_ex = (CONTEXT_EX *)(context_buffer + context_arch[test].context_length);
            ok(context_ex->Legacy.Offset == -(int)context_arch[test].context_length,
                    "Got unexpected Offset %d, flags %#x.\n", context_ex->Legacy.Offset, flags);
            ok(context_ex->Legacy.Length == ((flags & 0x20) ? context_arch[test].context_length
                    : context_arch[test].legacy_length),
                    "Got unexpected Length %#x, flags %#x.\n", context_ex->Legacy.Length, flags);
            ok(context_ex->All.Offset == -(int)context_arch[test].context_length,
                    "Got unexpected Offset %d, flags %#x.\n", context_ex->All.Offset, flags);

            /* No extra 8 bytes in x64 CONTEXT_EX here. */
            ok(context_ex->All.Length == context_arch[test].context_length + context_arch[1].context_ex_length,
                    "Got unexpected Length %#x, flags %#x.\n", context_ex->All.Length, flags);

            ok(context_ex->XState.Offset == 25,
                    "Got unexpected Offset %d, flags %#x.\n", context_ex->XState.Offset, flags);
            ok(!context_ex->XState.Length,
                    "Got unexpected Length %#x, flags %#x.\n", context_ex->XState.Length, flags);

            if (0)
            {
                /* Crashes on Windows. */
                pRtlLocateLegacyContext(NULL, NULL);
            }
            p = pRtlLocateLegacyContext(context_ex, NULL);
            ok(p == context, "Got unexpected p %p, flags %#x.\n", p, flags);
            length2 = 0xdeadbeef;
            p = pRtlLocateLegacyContext(context_ex, &length2);
            ok(p == context && length2 == context_ex->Legacy.Length,
                    "Got unexpected p %p, length %#x, flags %#x.\n", p, length2, flags);
            length2 = expected_length;

            if (0)
            {
                /* Crashes on Windows. */
                pGetXStateFeaturesMask(context, NULL);
                pRtlGetExtendedFeaturesMask(context_ex);
                pRtlSetExtendedFeaturesMask(context_ex, 0);
            }

            flags_fpx = flags & 0x10000 ? flags | 0x20 : flags | 0x8;

            mask = 0xdeadbeef;
            bret = pGetXStateFeaturesMask(context, &mask);
            SetLastError(0xdeadbeef);
            if (flags & CONTEXT_NATIVE)
                ok(bret && mask == ((flags & flags_fpx) == flags_fpx ? 0x3 : 0),
                        "Got unexpected bret %#x, mask %s, flags %#x.\n", bret, wine_dbgstr_longlong(mask), flags);
            else
                ok(!bret && mask == 0xdeadbeef && GetLastError() == 0xdeadbeef,
                        "Got unexpected bret %#x, mask %s, GetLastError() %#x, flags %#x.\n",
                        bret, wine_dbgstr_longlong(mask), GetLastError(), flags);

            bret = pSetXStateFeaturesMask(context, 0);
            ok(bret == !!(flags & CONTEXT_NATIVE), "Got unexpected bret %#x, flags %#x.\n", bret, flags);
            context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
            ok(context_flags == flags, "Got unexpected ContextFlags %#x, flags %#x.\n", context_flags, flags);

            bret = pSetXStateFeaturesMask(context, 1);
            ok(bret == !!(flags & CONTEXT_NATIVE), "Got unexpected bret %#x, flags %#x.\n", bret, flags);
            context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
            ok(context_flags == (bret ? flags_fpx : flags),
                    "Got unexpected ContextFlags %#x, flags %#x.\n", context_flags, flags);

            bret = pSetXStateFeaturesMask(context, 2);
            ok(bret == !!(flags & CONTEXT_NATIVE), "Got unexpected bret %#x, flags %#x.\n", bret, flags);
            context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
            ok(context_flags == (bret ? flags_fpx : flags),
                    "Got unexpected ContextFlags %#x, flags %#x.\n", context_flags, flags);

            bret = pSetXStateFeaturesMask(context, 4);
            ok(!bret, "Got unexpected bret %#x.\n", bret);
            context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
            ok(context_flags == (flags & CONTEXT_NATIVE ? flags_fpx : flags),
                    "Got unexpected ContextFlags %#x, flags %#x.\n", context_flags, flags);
            *(DWORD *)(context_buffer + context_arch[test].flags_offset) = flags;

            for (j = 0; j < context_arch[test].flags_offset; ++j)
            {
                if (context_buffer[j] != 0xcc)
                {
                    ok(0, "Buffer data changed at offset %#x.\n", j);
                    break;
                }
            }
            for (j = context_arch[test].flags_offset + sizeof(context_flags);
                    j < context_arch[test].context_length; ++j)
            {
                if (context_buffer[j] != 0xcc)
                {
                    ok(0, "Buffer data changed at offset %#x.\n", j);
                    break;
                }
            }
            for (j = context_arch[test].context_length + context_arch[test].context_ex_length;
                    j < sizeof(context_buffer); ++j)
            {
                if (context_buffer[j] != 0xcc)
                {
                    ok(0, "Buffer data changed at offset %#x.\n", j);
                    break;
                }
            }

            memset(context_buffer2, 0xcc, sizeof(context_buffer2));
            ret2 = pRtlInitializeExtendedContext(context_buffer2, flags, &context_ex);
            ok(!ret2, "Got unexpected ret2 %#x, flags %#x.\n", ret2, flags);
            ok(!memcmp(context_buffer2, context_buffer, sizeof(context_buffer2)),
                    "Context data do not match, flags %#x.\n", flags);

            memset(context_buffer2, 0xcc, sizeof(context_buffer2));
            ret2 = pRtlInitializeExtendedContext(context_buffer2 + 2, flags, &context_ex);
            ok(!ret2, "Got unexpected ret2 %#x, flags %#x.\n", ret2, flags);

            /* Buffer gets aligned to 16 bytes on x64, while returned context length suggests it should be 8. */
            align = test ? 4 : 16;
            ok(!memcmp(context_buffer2 + align, context_buffer,
                    sizeof(context_buffer2) - align),
                    "Context data do not match, flags %#x.\n", flags);

            SetLastError(0xdeadbeef);
            memset(context_buffer2, 0xcc, sizeof(context_buffer2));
            bret = pInitializeContext(context_buffer2 + 2, flags, &context, &length2);
            ok(bret && GetLastError() == 0xdeadbeef,
                    "Got unexpected bret %#x, GetLastError() %u, flags %#x.\n", bret, GetLastError(), flags);
            ok(length2 == expected_length, "Got unexpexted length %#x.\n", length);
            ok(!memcmp(context_buffer2 + align, context_buffer,
                    sizeof(context_buffer2) - align),
                    "Context data do not match, flags %#x.\n", flags);

            length2 = 0xdeadbeef;
            p = pLocateXStateFeature(context, 0, &length2);
            if (flags & CONTEXT_NATIVE)
                ok(p == (BYTE *)context + context_arch[test].xsavearea_offset
                        && length2 == offsetof(XSAVE_FORMAT, XmmRegisters),
                        "Got unexpected p %p, length %#x, flags %#x.\n", p, length2, flags);
            else
                ok(!p && length2 == 0xdeadbeef, "Got unexpected p %p, length %#x, flags %#x.\n", p, length2, flags);
            length2 = 0xdeadbeef;
            p = pLocateXStateFeature(context, 1, &length2);
            if (flags & CONTEXT_NATIVE)
                ok(p == (BYTE *)context + context_arch[test].xsavearea_offset + offsetof(XSAVE_FORMAT, XmmRegisters)
                        && length2 == sizeof(M128A) * context_arch[test].vector_reg_count,
                         "Got unexpected p %p, length %#x, flags %#x.\n", p, length2, flags);
            else
                ok(!p && length2 == 0xdeadbeef, "Got unexpected p %p, length %#x, flags %#x.\n", p, length2, flags);
            length2 = 0xdeadbeef;
            p = pLocateXStateFeature(context, 2, &length2);
            ok(!p && length2 == 0xdeadbeef, "Got unexpected p %p, length %#x, flags %#x.\n", p, length2, flags);

            if (!pRtlInitializeExtendedContext2 || !pInitializeContext2)
            {
                static int once;

                if (!once++)
                    win_skip("InitializeContext2 is not available.\n");
                continue;
            }

            length2 = expected_length;
            memset(context_buffer2, 0xcc, sizeof(context_buffer2));
            ret2 = pRtlInitializeExtendedContext2(context_buffer2 + 2, flags, &context_ex, ~(ULONG64)0);
            ok(!ret2, "Got unexpected ret2 %#x, flags %#x.\n", ret2, flags);
            ok(!memcmp(context_buffer2 + align, context_buffer,
                    sizeof(context_buffer2) - align),
                    "Context data do not match, flags %#x.\n", flags);

            memset(context_buffer2, 0xcc, sizeof(context_buffer2));
            bret = pInitializeContext2(context_buffer2 + 2, flags, &context, &length2, 0);
            ok(bret && GetLastError() == 0xdeadbeef,
                    "Got unexpected bret %#x, GetLastError() %u, flags %#x.\n", bret, GetLastError(), flags);
            ok(length2 == expected_length, "Got unexpexted length %#x.\n", length);
            ok(!memcmp(context_buffer2 + align, context_buffer,
                    sizeof(context_buffer2) - align),
                    "Context data do not match, flags %#x.\n", flags);

            length2 = 0xdeadbeef;
            p = pLocateXStateFeature(context, 0, &length2);
            if (flags & CONTEXT_NATIVE)
                ok(p == (BYTE *)context + context_arch[test].xsavearea_offset
                        && length2 == offsetof(XSAVE_FORMAT, XmmRegisters),
                        "Got unexpected p %p, length %#x, flags %#x.\n", p, length2, flags);
            else
                ok(!p && length2 == 0xdeadbeef, "Got unexpected p %p, length %#x, flags %#x.\n", p, length2, flags);
        }

        flags = context_arch[test].flag | 0x40;
        flags_fpx = flags & 0x10000 ? flags | 0x20 : flags | 0x8;

        length = 0xdeadbeef;
        ret = pRtlGetExtendedContextLength(flags, &length);

        if (!enabled_features)
        {
            ok(ret == STATUS_NOT_SUPPORTED && length == 0xdeadbeef,
                    "Got unexpected result ret %#x, length %#x.\n", ret, length);

            context_ex = (void *)0xdeadbeef;
            ret2 = pRtlInitializeExtendedContext(context_buffer, flags, &context_ex);
            ok(ret2 == STATUS_NOT_SUPPORTED, "Got unexpected result ret %#x, test %u.\n", ret2, test);

            SetLastError(0xdeadbeef);
            length2 = sizeof(context_buffer);
            bret = pInitializeContext(context_buffer, flags, &context, &length2);
            ok(bret && GetLastError() == 0xdeadbeef,
                    "Got unexpected bret %#x, GetLastError() %u, flags %#x.\n", bret, GetLastError(), flags);
            context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
            ok(context_flags == (flags & ~0x40), "Got unexpected ContextFlags %#x, flags %#x.\n",
                    context_flags, flags);

            if (pInitializeContext2)
            {
                SetLastError(0xdeadbeef);
                length2 = sizeof(context_buffer);
                bret = pInitializeContext2(context_buffer, flags, &context, &length2, ~(ULONG64)0);
                ok(bret && GetLastError() == 0xdeadbeef,
                        "Got unexpected bret %#x, GetLastError() %u, flags %#x.\n", bret, GetLastError(), flags);
                context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
                ok(context_flags == (flags & ~0x40), "Got unexpected ContextFlags %#x, flags %#x.\n",
                        context_flags, flags);
            }
            continue;
        }

        ok(!ret && length >= expected_length_xstate,
                "Got unexpected result ret %#x, length %#x, test %u.\n", ret, length, test);

        if (!pRtlGetExtendedContextLength2)
        {
            win_skip("RtlGetExtendedContextLength2 is not available.\n");
        }
        else
        {
            length = 0xdeadbeef;
            ret = pRtlGetExtendedContextLength2(flags, &length, 7);
            ok(!ret && length == expected_length_xstate,
                    "Got unexpected result ret %#x, length %#x, test %u.\n", ret, length, test);

            length = 0xdeadbeef;
            ret = pRtlGetExtendedContextLength2(flags, &length, ~0);
            ok(!ret && length >= expected_length_xstate,
                    "Got unexpected result ret %#x, length %#x, test %u.\n", ret, length, test);

            length = 0xdeadbeef;
            ret = pRtlGetExtendedContextLength2(flags, &length, 0);
            ok((!ret && length == expected_length_xstate - sizeof(YMMCONTEXT))
                    || broken(!ret && length == expected_length_xstate) /* win10pro */,
                    "Got unexpected result ret %#x, length %#x, test %u.\n", ret, length, test);

            length = 0xdeadbeef;
            ret = pRtlGetExtendedContextLength2(flags, &length, 3);
            ok((!ret && length == expected_length_xstate - sizeof(YMMCONTEXT))
                    || broken(!ret && length == expected_length_xstate) /* win10pro */,
                    "Got unexpected result ret %#x, length %#x, test %u.\n", ret, length, test);

            length = 0xdeadbeef;
            ret = pRtlGetExtendedContextLength2(flags, &length, 4);
            ok(!ret && length == expected_length_xstate,
                    "Got unexpected result ret %#x, length %#x, test %u.\n", ret, length, test);
        }

        pRtlGetExtendedContextLength(flags, &length);
        SetLastError(0xdeadbeef);
        bret = pInitializeContext(NULL, flags, NULL, &length2);
        ok(!bret && length2 == length && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
                "Got unexpected bret %#x, length2 %#x, GetLastError() %u, flags %#x.\n",
                bret, length2, GetLastError(), flags);

        SetLastError(0xdeadbeef);
        context = (void *)0xdeadbeef;
        length2 = length - 1;
        bret = pInitializeContext(context_buffer, flags, &context, &length2);
        ok(!bret && GetLastError() == ERROR_INSUFFICIENT_BUFFER && length2 == length && context == (void *)0xdeadbeef,
                "Got unexpected bret %#x, GetLastError() %u, length2 %#x, flags %#x.\n",
                bret, GetLastError(), length2, flags);

        SetLastError(0xdeadbeef);
        memset(context_buffer, 0xcc, sizeof(context_buffer));
        length2 = length + 1;
        bret = pInitializeContext(context_buffer, flags, &context, &length2);
        ok(bret && GetLastError() == 0xdeadbeef,
                "Got unexpected bret %#x, GetLastError() %u, flags %#x.\n", bret, GetLastError(), flags);
        ok(length2 == length, "Got unexpexted length %#x.\n", length);
        ok((BYTE *)context == context_buffer, "Got unexpected context %p.\n", context);

        context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
        ok(context_flags == flags, "Got unexpected ContextFlags %#x, flags %#x.\n", context_flags, flags);

        context_ex = (CONTEXT_EX *)(context_buffer + context_arch[test].context_length);
        ok(context_ex->Legacy.Offset == -(int)context_arch[test].context_length,
                "Got unexpected Offset %d, flags %#x.\n", context_ex->Legacy.Offset, flags);
        ok(context_ex->Legacy.Length == ((flags & 0x20) ? context_arch[test].context_length
                : context_arch[test].legacy_length),
                "Got unexpected Length %#x, flags %#x.\n", context_ex->Legacy.Length, flags);

        expected_offset = (((ULONG_PTR)context + context_arch[test].context_length
                + context_arch[test].context_ex_length + 63) & ~(ULONG64)63) - (ULONG_PTR)context
                - context_arch[test].context_length;
        ok(context_ex->XState.Offset == expected_offset,
                "Got unexpected Offset %d, flags %#x.\n", context_ex->XState.Offset, flags);
        ok(context_ex->XState.Length >= sizeof(XSTATE),
                "Got unexpected Length %#x, flags %#x.\n", context_ex->XState.Length, flags);

        ok(context_ex->All.Offset == -(int)context_arch[test].context_length,
                "Got unexpected Offset %d, flags %#x.\n", context_ex->All.Offset, flags);
        /* No extra 8 bytes in x64 CONTEXT_EX here. */
        ok(context_ex->All.Length == context_arch[test].context_length
                + context_ex->XState.Offset + context_ex->XState.Length,
                "Got unexpected Length %#x, flags %#x.\n", context_ex->All.Length, flags);

        xs = (XSTATE *)((BYTE *)context_ex + context_ex->XState.Offset);
        length2 = 0xdeadbeef;
        for (i = 0; i < 2; ++i)
        {
            p = pRtlLocateExtendedFeature(context_ex, i, &length2);
            ok(!p && length2 == 0xdeadbeef, "Got unexpected p %p, length %#x.\n", p, length2);
        }

        p = pRtlLocateExtendedFeature(context_ex, XSTATE_AVX, &length2);
        ok(length2 == sizeof(YMMCONTEXT), "Got unexpected length %#x.\n", length2);
        ok(p == &xs->YmmContext, "Got unexpected p %p.\n", p);
        p = pRtlLocateExtendedFeature(context_ex, XSTATE_AVX, NULL);
        ok(p == &xs->YmmContext, "Got unexpected p %p.\n", p);

        length2 = 0xdeadbeef;
        p = pLocateXStateFeature(context, 0, &length2);
        if (flags & CONTEXT_NATIVE)
            ok(p == (BYTE *)context + context_arch[test].xsavearea_offset
                    && length2 == offsetof(XSAVE_FORMAT, XmmRegisters),
                    "Got unexpected p %p, length %#x, flags %#x.\n", p, length2, flags);
        else
            ok(!p && length2 == 0xdeadbeef, "Got unexpected p %p, length %#x, flags %#x.\n", p, length2, flags);

        length2 = 0xdeadbeef;
        p = pLocateXStateFeature(context, 1, &length2);
        if (flags & CONTEXT_NATIVE)
            ok(p == (BYTE *)context + context_arch[test].xsavearea_offset + offsetof(XSAVE_FORMAT, XmmRegisters)
                    && length2 == sizeof(M128A) * context_arch[test].vector_reg_count,
                    "Got unexpected p %p, length %#x, flags %#x.\n", p, length2, flags);
        else
            ok(!p && length2 == 0xdeadbeef, "Got unexpected p %p, length %#x, flags %#x.\n", p, length2, flags);

        length2 = 0xdeadbeef;
        p = pLocateXStateFeature(context, 2, &length2);
        if (flags & CONTEXT_NATIVE)
            ok(p == &xs->YmmContext && length2 == sizeof(YMMCONTEXT),
                    "Got unexpected p %p, length %#x, flags %#x.\n", p, length2, flags);
        else
            ok(!p && length2 == 0xdeadbeef, "Got unexpected p %p, length %#x, flags %#x.\n", p, length2, flags);

        mask = 0xdeadbeef;
        bret = pGetXStateFeaturesMask(context, &mask);
        if (flags & CONTEXT_NATIVE)
            ok(bret && !mask,
                    "Got unexpected bret %#x, mask %s, flags %#x.\n", bret, wine_dbgstr_longlong(mask), flags);
        else
            ok(!bret && mask == 0xdeadbeef,
                    "Got unexpected bret %#x, mask %s, flags %#x.\n", bret, wine_dbgstr_longlong(mask), flags);

        expected_compaction = compaction_enabled ? ((ULONG64)1 << 63) | enabled_features : 0;
        ok(!xs->Mask, "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
        mask = pRtlGetExtendedFeaturesMask(context_ex);
        ok(mask == (xs->Mask & ~(ULONG64)3), "Got unexpected mask %s.\n", wine_dbgstr_longlong(mask));
        ok(xs->CompactionMask == expected_compaction,
                "Got unexpected CompactionMask %s.\n", wine_dbgstr_longlong(xs->CompactionMask));
        ok(!xs->Reserved[0], "Got unexpected Reserved[0]  %s.\n", wine_dbgstr_longlong(xs->Reserved[0]));

        xs->Mask = 0xdeadbeef;
        xs->CompactionMask = 0xdeadbeef;
        pRtlSetExtendedFeaturesMask(context_ex, 0);
        ok(!xs->Mask, "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
        mask = pRtlGetExtendedFeaturesMask(context_ex);
        ok(mask == (xs->Mask & ~(ULONG64)3), "Got unexpected mask %s.\n", wine_dbgstr_longlong(mask));
        ok(xs->CompactionMask == 0xdeadbeef, "Got unexpected CompactionMask %s.\n", wine_dbgstr_longlong(xs->CompactionMask));
        context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
        ok(context_flags == flags, "Got unexpected ContextFlags %#x, flags %#x.\n", context->ContextFlags, flags);

        xs->Mask = 0xdeadbeef;
        xs->CompactionMask = 0;
        pRtlSetExtendedFeaturesMask(context_ex, ~(ULONG64)0);
        ok(xs->Mask == (enabled_features & ~(ULONG64)3), "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
        mask = pRtlGetExtendedFeaturesMask(context_ex);
        ok(mask == (xs->Mask & ~(ULONG64)3), "Got unexpected mask %s.\n", wine_dbgstr_longlong(mask));
        ok(!xs->CompactionMask, "Got unexpected CompactionMask %s.\n",
                wine_dbgstr_longlong(xs->CompactionMask));
        context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
        ok(context_flags == flags, "Got unexpected ContextFlags %#x, flags %#x.\n", context->ContextFlags, flags);

        xs->Mask = 0xdeadbeef;
        xs->CompactionMask = 0xdeadbeef;
        bret = pSetXStateFeaturesMask(context, 7);
        ok(bret == !!(flags & CONTEXT_NATIVE), "Got unexpected bret %#x.\n", bret);
        context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
        ok(context_flags == (bret ? flags_fpx : flags),
                "Got unexpected ContextFlags %#x, flags %#x.\n", context_flags, flags);
        ok(xs->Mask == bret ? 4 : 0xdeadbeef, "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
        mask = pRtlGetExtendedFeaturesMask(context_ex);
        ok(mask == (xs->Mask & ~(ULONG64)3), "Got unexpected mask %s.\n", wine_dbgstr_longlong(mask));
        ok(xs->CompactionMask == bret ? expected_compaction : 0xdeadbeef, "Got unexpected CompactionMask %s.\n",
                wine_dbgstr_longlong(xs->CompactionMask));

        mask = 0xdeadbeef;
        bret = pGetXStateFeaturesMask(context, &mask);
        if (flags & CONTEXT_NATIVE)
            ok(bret && mask == enabled_features,
                    "Got unexpected bret %#x, mask %s, flags %#x.\n", bret, wine_dbgstr_longlong(mask), flags);
        else
            ok(!bret && mask == 0xdeadbeef,
                    "Got unexpected bret %#x, mask %s, flags %#x.\n", bret, wine_dbgstr_longlong(mask), flags);

        if (pRtlGetExtendedContextLength2)
        {
            memset(context_buffer, 0xcc, sizeof(context_buffer));
            pRtlGetExtendedContextLength2(flags, &length, 0);
            SetLastError(0xdeadbeef);
            memset(context_buffer, 0xcc, sizeof(context_buffer));
            length2 = length;
            bret = pInitializeContext2(context_buffer, flags, &context, &length2, 0);
            ok(bret && GetLastError() == 0xdeadbeef,
                    "Got unexpected bret %#x, GetLastError() %u, flags %#x.\n", bret, GetLastError(), flags);
            ok(length2 == length, "Got unexpexted length %#x.\n", length);
            ok((BYTE *)context == context_buffer, "Got unexpected context %p.\n", context);

            length2 = 0xdeadbeef;
            p = pLocateXStateFeature(context, 0, &length2);
            if (flags & CONTEXT_NATIVE)
                ok(p == (BYTE *)context + context_arch[test].xsavearea_offset
                    && length2 == offsetof(XSAVE_FORMAT, XmmRegisters),
                    "Got unexpected p %p, length %#x, flags %#x.\n", p, length2, flags);
            else
                ok(!p && length2 == 0xdeadbeef, "Got unexpected p %p, length %#x, flags %#x.\n", p, length2, flags);

            length2 = 0xdeadbeef;
            p = pRtlLocateExtendedFeature(context_ex, 2, &length2);
            ok((!p && length2 == sizeof(YMMCONTEXT))
                    || broken(p && length2 == sizeof(YMMCONTEXT)) /* win10pro */,
                    "Got unexpected p %p, length %#x, flags %#x.\n", p, length2, flags);

            length2 = 0xdeadbeef;
            p = pLocateXStateFeature(context, 2, &length2);
            ok(!p && length2 == (flags & CONTEXT_NATIVE) ? sizeof(YMMCONTEXT) : 0xdeadbeef,
                    "Got unexpected p %p, length %#x, flags %#x.\n", p, length2, flags);

            context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
            ok(context_flags == flags, "Got unexpected ContextFlags %#x, flags %#x.\n", context_flags, flags);

            context_ex = (CONTEXT_EX *)(context_buffer + context_arch[test].context_length);
            ok(context_ex->Legacy.Offset == -(int)context_arch[test].context_length,
                    "Got unexpected Offset %d, flags %#x.\n", context_ex->Legacy.Offset, flags);
            ok(context_ex->Legacy.Length == ((flags & 0x20) ? context_arch[test].context_length
                    : context_arch[test].legacy_length),
                    "Got unexpected Length %#x, flags %#x.\n", context_ex->Legacy.Length, flags);

            expected_offset = (((ULONG_PTR)context + context_arch[test].context_length
                    + context_arch[test].context_ex_length + 63) & ~(ULONG64)63) - (ULONG_PTR)context
                    - context_arch[test].context_length;
            ok(context_ex->XState.Offset == expected_offset,
                    "Got unexpected Offset %d, flags %#x.\n", context_ex->XState.Offset, flags);
            ok(context_ex->XState.Length == sizeof(XSTATE) - sizeof(YMMCONTEXT)
                    || broken(context_ex->XState.Length == sizeof(XSTATE)) /* win10pro */,
                    "Got unexpected Length %#x, flags %#x.\n", context_ex->XState.Length, flags);

            ok(context_ex->All.Offset == -(int)context_arch[test].context_length,
                    "Got unexpected Offset %d, flags %#x.\n", context_ex->All.Offset, flags);
            /* No extra 8 bytes in x64 CONTEXT_EX here. */
            ok(context_ex->All.Length == context_arch[test].context_length
                    + context_ex->XState.Offset + context_ex->XState.Length,
                    "Got unexpected Length %#x, flags %#x.\n", context_ex->All.Length, flags);

            expected_compaction = compaction_enabled ? (ULONG64)1 << 63 : 0;
            xs = (XSTATE *)((BYTE *)context_ex + context_ex->XState.Offset);
            ok(!xs->Mask, "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
            ok(xs->CompactionMask == expected_compaction,
                    "Got unexpected CompactionMask %s.\n", wine_dbgstr_longlong(xs->CompactionMask));
            ok(!xs->Reserved[0], "Got unexpected Reserved[0]  %s.\n", wine_dbgstr_longlong(xs->Reserved[0]));

            pRtlSetExtendedFeaturesMask(context_ex, ~(ULONG64)0);
            ok(xs->Mask == (enabled_features & ~(ULONG64)3), "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
            ok(xs->CompactionMask == expected_compaction, "Got unexpected CompactionMask %s.\n", wine_dbgstr_longlong(xs->CompactionMask));
        }
    }

    length = 0xdeadbeef;
    ret = pRtlGetExtendedContextLength(context_arch[0].flag | context_arch[1].flag, &length);
    ok(ret == STATUS_INVALID_PARAMETER && length == 0xdeadbeef, "Got unexpected result ret %#x, length %#x.\n",
            ret, length);

    if (0)
    {
        /* Crashes on Windows. */
        pRtlGetExtendedContextLength(CONTEXT_FULL, NULL);
        length = sizeof(context_buffer);
        pInitializeContext(context_buffer, CONTEXT_FULL, NULL, &length);
        pInitializeContext(context_buffer, CONTEXT_FULL, &context, NULL);
    }

    if (!(enabled_features & (1 << XSTATE_AVX)))
    {
        skip("AVX is not supported.\n");
        return;
    }

    /* Test RtlCaptureContext (doesn't support xstates). */
    length = sizeof(context_buffer);
    memset(context_buffer, 0xcc, sizeof(context_buffer));
    bret = pInitializeContext(context_buffer, CONTEXT_XSTATE, &context, &length);
    ok(bret, "Got unexpected bret %#x.\n", bret);
    context_ex = (CONTEXT_EX *)(context + 1);
    xs = (XSTATE *)((BYTE *)context_ex + context_ex->XState.Offset);

    *(void **)(call_func_code_set_ymm0 + call_func_offsets.func_addr) = RtlCaptureContext;
    *(void **)(call_func_code_set_ymm0 + call_func_offsets.func_param1) = context;
    *(void **)(call_func_code_set_ymm0 + call_func_offsets.func_param2) = (void *)0xdeadbeef;
    *(void **)(call_func_code_set_ymm0 + call_func_offsets.ymm0_save) = data;
    memcpy(code_mem, call_func_code_set_ymm0, sizeof(call_func_code_set_ymm0));

    memcpy(data, test_extended_context_data, sizeof(data));
    func();
    ok(context->ContextFlags == (CONTEXT_FULL | CONTEXT_SEGMENTS), "Got unexpected ContextFlags %#x.\n",
            context->ContextFlags);
    for (i = 0; i < 8; ++i)
        ok(data[i] == test_extended_context_data[i], "Got unexpected data %#x, i %u.\n", data[i], i);

    /* Test GetThreadContext (current thread, ymm0 set). */
    length = sizeof(context_buffer);
    memset(context_buffer, 0xcc, sizeof(context_buffer));
    bret = pInitializeContext(context_buffer, CONTEXT_FULL | CONTEXT_XSTATE | CONTEXT_FLOATING_POINT,
            &context, &length);
    ok(bret, "Got unexpected bret %#x.\n", bret);
    memset(&xs->YmmContext, 0xcc, sizeof(xs->YmmContext));

    expected_flags = CONTEXT_FULL | CONTEXT_XSTATE | CONTEXT_FLOATING_POINT;
#ifdef __i386__
    expected_flags |= CONTEXT_EXTENDED_REGISTERS;
#endif
    pSetXStateFeaturesMask(context, ~(ULONG64)0);
    ok(context->ContextFlags == expected_flags, "Got unexpected ContextFlags %#x.\n",
            context->ContextFlags);
    *(void **)(call_func_code_set_ymm0 + call_func_offsets.func_addr) = GetThreadContext;
    *(void **)(call_func_code_set_ymm0 + call_func_offsets.func_param1) = (void *)GetCurrentThread();
    *(void **)(call_func_code_set_ymm0 + call_func_offsets.func_param2) = context;
    *(void **)(call_func_code_set_ymm0 + call_func_offsets.ymm0_save) = data;
    memcpy(code_mem, call_func_code_set_ymm0, sizeof(call_func_code_set_ymm0));
    xs->CompactionMask = 2;
    if (!compaction_enabled)
        xs->Mask = 0;
    context_ex->XState.Length = sizeof(XSTATE);

    bret = func();
    ok(bret, "Got unexpected bret %#x, GetLastError() %u.\n", bret, GetLastError());

    ok(context->ContextFlags == expected_flags, "Got unexpected ContextFlags %#x.\n",
            context->ContextFlags);
    expected_compaction = compaction_enabled ? (ULONG64)1 << 63 : 0;

    ok(!xs->Mask || broken(xs->Mask == 4) /* win10pro */,
            "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
    ok(xs->CompactionMask == expected_compaction, "Got unexpected CompactionMask %s.\n",
            wine_dbgstr_longlong(xs->CompactionMask));

    for (i = 4; i < 8; ++i)
        ok(data[i] == test_extended_context_data[i], "Got unexpected data %#x, i %u.\n", data[i], i);

    for (i = 0; i < 4; ++i)
        ok(((ULONG *)&xs->YmmContext)[i] == (xs->Mask == 4 ? test_extended_context_data[i + 4] : 0xcccccccc),
                "Got unexpected data %#x, i %u.\n", ((ULONG *)&xs->YmmContext)[i], i);

    expected_compaction = compaction_enabled ? ((ULONG64)1 << 63) | 4 : 0;

    xs->CompactionMask = 4;
    xs->Mask = compaction_enabled ? 0 : 4;
    context_ex->XState.Length = sizeof(XSTATE) + 64;
    bret = func();
    ok(!bret && GetLastError() == ERROR_INVALID_PARAMETER,
            "Got unexpected bret %#x, GetLastError() %u.\n", bret, GetLastError());
    ok(context->ContextFlags == expected_flags, "Got unexpected ContextFlags %#x.\n",
            context->ContextFlags);
    ok(xs->Mask == (compaction_enabled ? 0 : 4), "Got unexpected Mask %#I64x.\n", xs->Mask);
    ok(xs->CompactionMask == 4, "Got unexpected CompactionMask %s.\n",
            wine_dbgstr_longlong(xs->CompactionMask));
    for (i = 0; i < 4; ++i)
        ok(((ULONG *)&xs->YmmContext)[i] == 0xcccccccc
                || broken(((ULONG *)&xs->YmmContext)[i] == test_extended_context_data[i + 4]) /* win10pro */,
                "Got unexpected data %#x, i %u.\n", ((ULONG *)&xs->YmmContext)[i], i);

    xs->CompactionMask = 4;
    xs->Mask = compaction_enabled ? 0 : 4;
    context_ex->XState.Length = offsetof(XSTATE, YmmContext);
    bret = func();
    ok(context->ContextFlags == expected_flags, "Got unexpected ContextFlags %#x.\n",
            context->ContextFlags);
    ok(!bret && GetLastError() == ERROR_MORE_DATA,
            "Got unexpected bret %#x, GetLastError() %u.\n", bret, GetLastError());
    ok(xs->Mask == 4, "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
    ok(xs->CompactionMask == expected_compaction, "Got unexpected CompactionMask %s.\n",
            wine_dbgstr_longlong(xs->CompactionMask));
    for (i = 0; i < 4; ++i)
        ok(((ULONG *)&xs->YmmContext)[i] == 0xcccccccc
                || broken(((ULONG *)&xs->YmmContext)[i] == test_extended_context_data[i + 4]) /* win10pro */,
                "Got unexpected data %#x, i %u.\n", ((ULONG *)&xs->YmmContext)[i], i);

    context_ex->XState.Length = sizeof(XSTATE);
    xs->CompactionMask = 4;
    xs->Mask = compaction_enabled ? 0 : 4;
    bret = func();
    ok(bret, "Got unexpected bret %#x, GetLastError() %u.\n", bret, GetLastError());

    ok(context->ContextFlags == expected_flags, "Got unexpected ContextFlags %#x.\n",
            context->ContextFlags);

    ok(xs->Mask == 4, "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
    ok(xs->CompactionMask == expected_compaction, "Got unexpected CompactionMask %s.\n",
            wine_dbgstr_longlong(xs->CompactionMask));

    for (i = 4; i < 8; ++i)
        ok(data[i] == test_extended_context_data[i], "Got unexpected data %#x, i %u.\n", data[i], i);

    for (i = 0; i < 4; ++i)
        ok(((ULONG *)&xs->YmmContext)[i] == test_extended_context_data[i + 4],
                "Got unexpected data %#x, i %u.\n", ((ULONG *)&xs->YmmContext)[i], i);

    /* Test GetThreadContext (current thread, ymm state cleared). */
    length = sizeof(context_buffer);
    memset(context_buffer, 0xcc, sizeof(context_buffer));
    bret = pInitializeContext(context_buffer, CONTEXT_FULL | CONTEXT_XSTATE | CONTEXT_FLOATING_POINT,
            &context, &length);
    memset(&xs->YmmContext, 0xcc, sizeof(xs->YmmContext));
    ok(bret, "Got unexpected bret %#x.\n", bret);
    pSetXStateFeaturesMask(context, ~(ULONG64)0);
    *(void **)(call_func_code_reset_ymm_state + call_func_offsets.func_addr) = GetThreadContext;
    *(void **)(call_func_code_reset_ymm_state + call_func_offsets.func_param1) = (void *)GetCurrentThread();
    *(void **)(call_func_code_reset_ymm_state + call_func_offsets.func_param2) = context;
    *(void **)(call_func_code_reset_ymm_state + call_func_offsets.ymm0_save) = data;
    memcpy(code_mem, call_func_code_reset_ymm_state, sizeof(call_func_code_reset_ymm_state));

    bret = func();
    ok(bret, "Got unexpected bret %#x, GetLastError() %u.\n", bret, GetLastError());

    expected_flags = CONTEXT_FULL | CONTEXT_XSTATE | CONTEXT_FLOATING_POINT;
#ifdef __i386__
    expected_flags |= CONTEXT_EXTENDED_REGISTERS;
#endif
    ok(context->ContextFlags == expected_flags, "Got unexpected ContextFlags %#x.\n",
            context->ContextFlags);

    expected_compaction = compaction_enabled ? ((ULONG64)1 << 63) | 4 : 0;

    xs = (XSTATE *)((BYTE *)context_ex + context_ex->XState.Offset);
    ok(xs->Mask == (xsaveopt_enabled ? 0 : 4), "Got unexpected Mask %#I64x.\n", xs->Mask);
    ok(xs->CompactionMask == expected_compaction, "Got unexpected CompactionMask %s.\n",
            wine_dbgstr_longlong(xs->CompactionMask));

    for (i = 4; i < 8; ++i)
        ok(!data[i], "Got unexpected data %#x, i %u.\n", data[i], i);

    for (i = 0; i < 4; ++i)
        ok(((ULONG *)&xs->YmmContext)[i] == ((xs->Mask & 4) ? 0 : 0xcccccccc)
                || broken(((ULONG *)&xs->YmmContext)[i] == test_extended_context_data[i + 4]),
                "Got unexpected data %#x, i %u.\n", ((ULONG *)&xs->YmmContext)[i], i);

    /* Test fault exception context. */
    memset(data, 0xff, sizeof(data));
    xs->Mask = 0;
    test_extended_context_modified_state = FALSE;
    run_exception_test(test_extended_context_handler, NULL, except_code_reset_ymm_state,
            ARRAY_SIZE(except_code_reset_ymm_state), PAGE_EXECUTE_READ);
    for (i = 0; i < 8; ++i)
    {
        /* Older Windows version do not reset AVX context to INIT_STATE on x86. */
        ok(!data[i] || broken(i >= 4 && sizeof(void *) == 4 && data[i] == test_extended_context_spoil_data2[i]),
                "Got unexpected data %#x, i %u.\n", data[i], i);
    }

    memcpy(data, test_extended_context_data, sizeof(data));
    test_extended_context_modified_state = TRUE;
    run_exception_test(test_extended_context_handler, NULL, except_code_set_ymm0,
            ARRAY_SIZE(except_code_set_ymm0), PAGE_EXECUTE_READ);

    for (i = 0; i < 8; ++i)
        ok(data[i] == test_extended_context_data[i], "Got unexpected data %#x, i %u.\n", data[i], i);

    /* Test GetThreadContext for the other thread. */
    thread = CreateThread(NULL, 0, test_extended_context_thread, 0, CREATE_SUSPENDED, NULL);
    ok(!!thread, "Failed to create thread.\n");

    bret = pInitializeContext(context_buffer, CONTEXT_FULL | CONTEXT_XSTATE | CONTEXT_FLOATING_POINT,
            &context, &length);
    ok(bret, "Got unexpected bret %#x.\n", bret);
    memset(&xs->YmmContext, 0xcc, sizeof(xs->YmmContext));
    context_ex = (CONTEXT_EX *)(context + 1);
    xs = (XSTATE *)((BYTE *)context_ex + context_ex->XState.Offset);
    pSetXStateFeaturesMask(context, 4);

    bret = GetThreadContext(thread, context);
    ok(bret, "Got unexpected bret %#x, GetLastError() %u.\n", bret, GetLastError());
    todo_wine_if (!xsaveopt_enabled)
        ok(xs->Mask == (xsaveopt_enabled ? 0 : 4), "Got unexpected Mask %#I64x.\n", xs->Mask);
    ok(xs->CompactionMask == expected_compaction, "Got unexpected CompactionMask %s.\n",
            wine_dbgstr_longlong(xs->CompactionMask));
    for (i = 0; i < 16 * 4; ++i)
        ok(((ULONG *)&xs->YmmContext)[i] == ((xs->Mask & 4) ? 0 : 0xcccccccc),
                "Got unexpected value %#x, i %u.\n", ((ULONG *)&xs->YmmContext)[i], i);

    pSetXStateFeaturesMask(context, 4);
    memset(&xs->YmmContext, 0, sizeof(xs->YmmContext));
    bret = SetThreadContext(thread, context);
    ok(bret, "Got unexpected bret %#x, GetLastError() %u.\n", bret, GetLastError());

    memset(&xs->YmmContext, 0xcc, sizeof(xs->YmmContext));
    bret = GetThreadContext(thread, context);
    ok(bret, "Got unexpected bret %#x, GetLastError() %u.\n", bret, GetLastError());
    ok(!xs->Mask || broken(xs->Mask == 4), "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
    ok(xs->CompactionMask == expected_compaction, "Got unexpected CompactionMask %s.\n",
            wine_dbgstr_longlong(xs->CompactionMask));
    for (i = 0; i < 16 * 4; ++i)
        ok(((ULONG *)&xs->YmmContext)[i] == 0xcccccccc || broken(xs->Mask == 4 && !((ULONG *)&xs->YmmContext)[i]),
                "Got unexpected value %#x, i %u.\n", ((ULONG *)&xs->YmmContext)[i], i);

    pSetXStateFeaturesMask(context, 4);
    memset(&xs->YmmContext, 0x28, sizeof(xs->YmmContext));
    bret = SetThreadContext(thread, context);
    ok(bret, "Got unexpected bret %#x, GetLastError() %u.\n", bret, GetLastError());
    memset(&xs->YmmContext, 0xcc, sizeof(xs->YmmContext));
    bret = GetThreadContext(thread, context);
    ok(bret, "Got unexpected bret %#x, GetLastError() %u.\n", bret, GetLastError());
    ok(xs->Mask == 4, "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
    ok(xs->CompactionMask == expected_compaction, "Got unexpected CompactionMask %s.\n",
            wine_dbgstr_longlong(xs->CompactionMask));
    for (i = 0; i < 16 * 4; ++i)
        ok(((ULONG *)&xs->YmmContext)[i] == 0x28282828, "Got unexpected value %#x, i %u.\n",
                ((ULONG *)&xs->YmmContext)[i], i);

    wait_for_thread_next_suspend(thread);

    bret = GetThreadContext(thread, context);
    ok(bret, "Got unexpected bret %#x, GetLastError() %u.\n", bret, GetLastError());
    pSetXStateFeaturesMask(context, 4);
    memset(&xs->YmmContext, 0x48, sizeof(xs->YmmContext));
    bret = SetThreadContext(thread, context);
    ok(bret, "Got unexpected bret %#x, GetLastError() %u.\n", bret, GetLastError());

    wait_for_thread_next_suspend(thread);

    memset(&xs->YmmContext, 0xcc, sizeof(xs->YmmContext));
    bret = GetThreadContext(thread, context);
    ok(bret, "Got unexpected bret %#x, GetLastError() %u.\n", bret, GetLastError());
    ok(xs->Mask == 4, "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));

    for (i = 0; i < 4; ++i)
        ok(((ULONG *)&xs->YmmContext)[i] == 0x68686868, "Got unexpected value %#x, i %u.\n",
                ((ULONG *)&xs->YmmContext)[i], i);

    wait_for_thread_next_suspend(thread);

    memset(&xs->YmmContext, 0xcc, sizeof(xs->YmmContext));
    bret = GetThreadContext(thread, context);
    ok(bret, "Got unexpected bret %#x, GetLastError() %u.\n", bret, GetLastError());
    todo_wine_if (!xsaveopt_enabled && sizeof(void *) != 4)
        ok(xs->Mask == (xsaveopt_enabled ? 0 : 4) || (sizeof(void *) == 4 && xs->Mask == 4),
                "Got unexpected Mask %#I64x.\n", xs->Mask);
    if (xs->Mask == 4)
    {
        for (i = 0; i < 8 * sizeof(void *); ++i)
            ok(((ULONG *)&xs->YmmContext)[i] == 0,
                    "Got unexpected value %#x, i %u.\n", ((ULONG *)&xs->YmmContext)[i], i);
        for (; i < 16 * 4; ++i)
            ok(((ULONG *)&xs->YmmContext)[i] == 0x48484848,
                    "Got unexpected value %#x, i %u.\n", ((ULONG *)&xs->YmmContext)[i], i);
    }
    else
    {
        for (i = 0; i < 16 * 4; ++i)
            ok(((ULONG *)&xs->YmmContext)[i] == 0xcccccccc,
                    "Got unexpected value %#x, i %u.\n", ((ULONG *)&xs->YmmContext)[i], i);
    }

    bret = ResumeThread(thread);
    ok(bret, "Got unexpected bret %#x, GetLastError() %u.\n", bret, GetLastError());

    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
}

struct modified_range
{
    ULONG start;
    ULONG flag;
};

#define check_changes_in_range(a, b, c, d) check_changes_in_range_(__FILE__, __LINE__, a, b, c, d)
static void check_changes_in_range_(const char *file, unsigned int line, const BYTE *p,
        const struct modified_range *range, ULONG flags, unsigned int length)
{
    ULONG range_flag, flag;
    unsigned int once = 0;
    unsigned int i;

    range_flag = 0;
    for (i = 0; i < length; i++)
    {
        if (i == range->start)
        {
            range_flag = range->flag;
            ++range;
        }

        if ((flag = range_flag) == ~0)
            continue;

        if (flag & 0x80000000)
        {
            if (flag & flags && p[i] == 0xcc)
            {
                if (!once++)
                    ok(broken(1), "Matched broken result at %#x, flags %#x.\n", i, flags);
                continue;
            }
            flag = 0;
        }

        if (flag & flags && p[i] != 0xcc)
        {
            ok_(file, line)(0, "Got unexpected byte %#x at %#x, flags %#x.\n", p[i], i, flags);
            return;
        }
        else if (!(flag & flags) && p[i] != 0xdd)
        {
            ok_(file, line)(0, "Got unexpected byte %#x at %#x, flags %#x.\n", p[i], i, flags);
            return;
        }
    }
    ok_(file, line)(1, "Range matches.\n");
}

static void test_copy_context(void)
{
    static const struct modified_range ranges_amd64[] =
    {
        {0x30, ~0}, {0x38, 0x1}, {0x3a, 0x4}, {0x42, 0x1}, {0x48, 0x10}, {0x78, 0x2}, {0x98, 0x1},
        {0xa0, 0x2}, {0xf8, 0x1}, {0x100, 0x8}, {0x2a0, 0x80000008}, {0x4b0, 0x10}, {0x4d0, ~0},
        {0x4e8, 0}, {0x500, ~0}, {0x640, 0}, {0x1000, 0},
    };
    static const struct modified_range ranges_x86[] =
    {
        {0x0, ~0}, {0x4, 0x10}, {0x1c, 0x8}, {0x8c, 0x4}, {0x9c, 0x2}, {0xb4, 0x1}, {0xcc, 0x20}, {0x1ec, 0x80000020},
        {0x2cc, ~0}, {0x294, 0}, {0x1000, 0},
    };
    static const struct modified_range single_range[] =
    {
        {0x0, 0x1}, {0x1000, 0},
    };

    static const ULONG tests[] =
    {
        /* AMD64 */
        CONTEXT_AMD64_CONTROL,
        CONTEXT_AMD64_INTEGER,
        CONTEXT_AMD64_SEGMENTS,
        CONTEXT_AMD64_FLOATING_POINT,
        CONTEXT_AMD64_DEBUG_REGISTERS,
        CONTEXT_AMD64_FULL,
        CONTEXT_AMD64_XSTATE,
        CONTEXT_AMD64_ALL,
        /* X86 */
        CONTEXT_I386_CONTROL,
        CONTEXT_I386_INTEGER,
        CONTEXT_I386_SEGMENTS,
        CONTEXT_I386_FLOATING_POINT,
        CONTEXT_I386_DEBUG_REGISTERS,
        CONTEXT_I386_EXTENDED_REGISTERS,
        CONTEXT_I386_XSTATE,
        CONTEXT_I386_ALL
    };
    static const ULONG arch_flags[] = {CONTEXT_AMD64, CONTEXT_i386};

    DECLSPEC_ALIGN(64) BYTE src_context_buffer[4096];
    DECLSPEC_ALIGN(64) BYTE dst_context_buffer[4096];
    ULONG64 enabled_features, expected_compaction;
    unsigned int context_length, flags_offset, i;
    CONTEXT_EX *src_ex, *dst_ex;
    XSTATE *dst_xs, *src_xs;
    BOOL compaction, bret;
    CONTEXT *src, *dst;
    NTSTATUS status;
    DWORD length;
    ULONG flags;

    if (!pRtlCopyExtendedContext)
    {
        win_skip("RtlCopyExtendedContext is not available.\n");
        return;
    }

    if (!pRtlGetEnabledExtendedFeatures)
    {
        skip("RtlGetEnabledExtendedFeatures is not available.\n");
        return;
    }

    enabled_features = pRtlGetEnabledExtendedFeatures(~(ULONG64)0);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        flags = tests[i];
        flags_offset = (flags & CONTEXT_AMD64) ? offsetof(AMD64_CONTEXT,ContextFlags)
                                               : offsetof(I386_CONTEXT,ContextFlags);

        memset(dst_context_buffer, 0xdd, sizeof(dst_context_buffer));
        memset(src_context_buffer, 0xcc, sizeof(src_context_buffer));

        status = pRtlInitializeExtendedContext(src_context_buffer, flags, &src_ex);
        if (enabled_features || !(flags & 0x40))
        {
            ok(!status, "Got unexpected status %#x, flags %#x.\n", status, flags);
        }
        else
        {
            ok(status == STATUS_NOT_SUPPORTED, "Got unexpected status %#x, flags %#x.\n", status, flags);
            continue;
        }
        status = pRtlInitializeExtendedContext(dst_context_buffer, flags, &dst_ex);
        ok(!status, "Got unexpected status %#x, flags %#x.\n", status, flags);

        src = pRtlLocateLegacyContext(src_ex, NULL);
        dst = pRtlLocateLegacyContext(dst_ex, NULL);

        *(DWORD *)((BYTE *)dst + flags_offset) = 0;
        *(DWORD *)((BYTE *)src + flags_offset) = 0;

        src_xs = (XSTATE *)((BYTE *)src_ex + src_ex->XState.Offset);
        memset(src_xs, 0xcc, sizeof(XSTATE));
        src_xs->Mask = 3;
        src_xs->CompactionMask = ~(ULONG64)0;

        status = pRtlCopyExtendedContext(dst_ex, flags, src_ex);
        ok(!status, "Got unexpected status %#x, flags %#x.\n", status, flags);

        context_length = (BYTE *)dst_ex - (BYTE *)dst + dst_ex->All.Length;
        check_changes_in_range((BYTE *)dst, flags & CONTEXT_AMD64 ? &ranges_amd64[0] : &ranges_x86[0],
                flags, context_length);

        ok(*(DWORD *)((BYTE *)dst + flags_offset) == flags, "Got unexpected ContextFlags %#x, flags %#x.\n",
                *(DWORD *)((BYTE *)dst + flags_offset), flags);

        memset(dst_context_buffer, 0xdd, sizeof(dst_context_buffer));
        status = pRtlInitializeExtendedContext(dst_context_buffer, flags, &dst_ex);
        ok(!status, "Got unexpected status %#x, flags %#x.\n", status, flags);
        *(DWORD *)((BYTE *)src + flags_offset) = 0;
        *(DWORD *)((BYTE *)dst + flags_offset) = 0;
        SetLastError(0xdeadbeef);
        status = pRtlCopyContext(dst, flags | 0x40, src);
        ok(status == (enabled_features ? STATUS_INVALID_PARAMETER : STATUS_NOT_SUPPORTED)
           || broken(status == STATUS_INVALID_PARAMETER),
           "Got unexpected status %#x, flags %#x.\n", status, flags);
        ok(*(DWORD *)((BYTE *)dst + flags_offset) == 0, "Got unexpected ContextFlags %#x, flags %#x.\n",
                *(DWORD *)((BYTE *)dst + flags_offset), flags);
        check_changes_in_range((BYTE *)dst, flags & CONTEXT_AMD64 ? &ranges_amd64[0] : &ranges_x86[0],
                0, context_length);

        *(DWORD *)((BYTE *)dst + flags_offset) = flags & (CONTEXT_AMD64 | CONTEXT_i386);
        *(DWORD *)((BYTE *)src + flags_offset) = flags;
        status = pRtlCopyContext(dst, flags, src);
        if (flags & 0x40)
            ok((status == STATUS_BUFFER_OVERFLOW)
               || broken(!(flags & CONTEXT_NATIVE) && status == STATUS_INVALID_PARAMETER),
               "Got unexpected status %#x, flags %#x.\n", status, flags);
        else
            ok(!status || broken(!(flags & CONTEXT_NATIVE) && status == STATUS_INVALID_PARAMETER),
               "Got unexpected status %#x, flags %#x.\n", status, flags);
        if (!status)
        {
            ok(*(DWORD *)((BYTE *)dst + flags_offset) == flags, "Got unexpected ContextFlags %#x, flags %#x.\n",
                    *(DWORD *)((BYTE *)dst + flags_offset), flags);
            check_changes_in_range((BYTE *)dst, flags & CONTEXT_AMD64 ? &ranges_amd64[0] : &ranges_x86[0],
                    flags, context_length);
        }
        else
        {
            ok(*(DWORD *)((BYTE *)dst + flags_offset) == (flags & 0x110000),
                    "Got unexpected ContextFlags %#x, flags %#x.\n",
                    *(DWORD *)((BYTE *)dst + flags_offset), flags);
            check_changes_in_range((BYTE *)dst, flags & CONTEXT_AMD64 ? &ranges_amd64[0] : &ranges_x86[0],
                    0, context_length);
        }
    }

    for (i = 0; i < ARRAY_SIZE(arch_flags); ++i)
    {
        flags = arch_flags[i] | 0x42;
        flags_offset = (flags & CONTEXT_AMD64) ? offsetof(AMD64_CONTEXT,ContextFlags)
                                               : offsetof(I386_CONTEXT,ContextFlags);
        context_length = (flags & CONTEXT_AMD64) ? sizeof(AMD64_CONTEXT) : sizeof(I386_CONTEXT);

        memset(dst_context_buffer, 0xdd, sizeof(dst_context_buffer));
        memset(src_context_buffer, 0xcc, sizeof(src_context_buffer));
        length = sizeof(src_context_buffer);
        bret = pInitializeContext(src_context_buffer, flags, &src, &length);
        ok(bret, "Got unexpected bret %#x, flags %#x.\n", bret, flags);

        length = sizeof(dst_context_buffer);
        bret = pInitializeContext(dst_context_buffer, flags, &dst, &length);
        ok(bret, "Got unexpected bret %#x, flags %#x.\n", bret, flags);

        dst_ex = (CONTEXT_EX *)((BYTE *)dst + context_length);
        src_ex = (CONTEXT_EX *)((BYTE *)src + context_length);

        dst_xs = (XSTATE *)((BYTE *)dst_ex + dst_ex->XState.Offset);
        src_xs = (XSTATE *)((BYTE *)src_ex + src_ex->XState.Offset);

        *(DWORD *)((BYTE *)dst + flags_offset) = 0;
        *(DWORD *)((BYTE *)src + flags_offset) = 0;

        compaction = !!(src_xs->CompactionMask & ((ULONG64)1 << 63));
        expected_compaction = (compaction ? ((ULONG64)1 << (ULONG64)63) | enabled_features : 0);

        memset(&src_xs->YmmContext, 0xcc, sizeof(src_xs->YmmContext));
        src_xs->CompactionMask = ~(ULONG64)0;

        src_xs->Mask = 0;
        memset(&dst_xs->YmmContext, 0xdd, sizeof(dst_xs->YmmContext));
        dst_xs->CompactionMask = 0xdddddddddddddddd;
        dst_xs->Mask = 0xdddddddddddddddd;
        dst_ex->XState.Length = 0;
        status = pRtlCopyExtendedContext(dst_ex, flags, src_ex);
        ok(status == (enabled_features ? STATUS_BUFFER_OVERFLOW : STATUS_NOT_SUPPORTED),
                "Got unexpected status %#x, flags %#x.\n", status, flags);

        if (!enabled_features)
            continue;

        ok(*(DWORD *)((BYTE *)dst + flags_offset) == flags, "Got unexpected ContextFlags %#x, flags %#x.\n",
                *(DWORD *)((BYTE *)dst + flags_offset), flags);

        src_xs->Mask = ~(ULONG64)0;

        memset(&dst_xs->YmmContext, 0xdd, sizeof(dst_xs->YmmContext));
        dst_xs->CompactionMask = 0xdddddddddddddddd;
        dst_xs->Mask = 0xdddddddddddddddd;
        dst_ex->XState.Length = 0;
        status = pRtlCopyExtendedContext(dst_ex, flags, src_ex);
        ok(status == STATUS_BUFFER_OVERFLOW, "Got unexpected status %#x, flags %#x.\n", status, flags);
        ok(*(DWORD *)((BYTE *)dst + flags_offset) == flags, "Got unexpected ContextFlags %#x, flags %#x.\n",
                *(DWORD *)((BYTE *)dst + flags_offset), flags);

        ok(dst_xs->Mask == 0xdddddddddddddddd, "Got unexpected Mask %s.\n",
                wine_dbgstr_longlong(dst_xs->Mask));
        ok(dst_xs->CompactionMask == 0xdddddddddddddddd, "Got unexpected CompactionMask %s.\n",
                wine_dbgstr_longlong(dst_xs->CompactionMask));
        check_changes_in_range((BYTE *)&dst_xs->YmmContext, single_range, 0, sizeof(dst_xs->YmmContext));

        src_xs->Mask = 3;
        memset(&dst_xs->YmmContext, 0xdd, sizeof(dst_xs->YmmContext));
        dst_xs->CompactionMask = 0xdddddddddddddddd;
        dst_xs->Mask = 0xdddddddddddddddd;
        dst_ex->XState.Length = offsetof(XSTATE, YmmContext);
        status = pRtlCopyExtendedContext(dst_ex, flags, src_ex);
        ok(!status, "Got unexpected status %#x, flags %#x.\n", status, flags);
        ok(*(DWORD *)((BYTE *)dst + flags_offset) == flags, "Got unexpected ContextFlags %#x, flags %#x.\n",
                *(DWORD *)((BYTE *)dst + flags_offset), flags);
        ok(dst_xs->Mask == 0, "Got unexpected Mask %s.\n",
                wine_dbgstr_longlong(dst_xs->Mask));
        ok(dst_xs->CompactionMask == expected_compaction,
                "Got unexpected CompactionMask %s.\n", wine_dbgstr_longlong(dst_xs->CompactionMask));
        check_changes_in_range((BYTE *)&dst_xs->YmmContext, single_range, 0, sizeof(dst_xs->YmmContext));

        memset(&dst_xs->YmmContext, 0xdd, sizeof(dst_xs->YmmContext));
        dst_xs->CompactionMask = 0xdddddddddddddddd;
        dst_xs->Mask = 0xdddddddddddddddd;
        dst_ex->XState.Length = sizeof(XSTATE);
        status = pRtlCopyExtendedContext(dst_ex, flags, src_ex);
        ok(!status, "Got unexpected status %#x, flags %#x.\n", status, flags);
        ok(dst_xs->Mask == 0, "Got unexpected Mask %s.\n",
                wine_dbgstr_longlong(dst_xs->Mask));
        ok(dst_xs->CompactionMask == expected_compaction,
                "Got unexpected CompactionMask %s.\n", wine_dbgstr_longlong(dst_xs->CompactionMask));
        check_changes_in_range((BYTE *)&dst_xs->YmmContext, single_range, 0, sizeof(dst_xs->YmmContext));

        src_xs->Mask = 4;
        memset(&dst_xs->YmmContext, 0xdd, sizeof(dst_xs->YmmContext));
        dst_xs->CompactionMask = 0xdddddddddddddddd;
        dst_xs->Mask = 0xdddddddddddddddd;
        status = pRtlCopyExtendedContext(dst_ex, flags, src_ex);
        ok(!status, "Got unexpected status %#x, flags %#x.\n", status, flags);
        ok(dst_xs->Mask == 4, "Got unexpected Mask %s.\n",
                wine_dbgstr_longlong(dst_xs->Mask));
        ok(dst_xs->CompactionMask == expected_compaction,
                "Got unexpected CompactionMask %s.\n", wine_dbgstr_longlong(dst_xs->CompactionMask));
        check_changes_in_range((BYTE *)&dst_xs->YmmContext, single_range, 1, sizeof(dst_xs->YmmContext));

        src_xs->Mask = 3;
        memset(&dst_xs->YmmContext, 0xdd, sizeof(dst_xs->YmmContext));
        dst_xs->CompactionMask = 0xdddddddddddddddd;
        dst_xs->Mask = 0xdddddddddddddddd;
        status = pRtlCopyExtendedContext(dst_ex, flags, src_ex);
        ok(!status, "Got unexpected status %#x, flags %#x.\n", status, flags);
        ok(dst_xs->Mask == 0, "Got unexpected Mask %s.\n",
                wine_dbgstr_longlong(dst_xs->Mask));
        ok(dst_xs->CompactionMask == expected_compaction,
                "Got unexpected CompactionMask %s.\n", wine_dbgstr_longlong(dst_xs->CompactionMask));
        check_changes_in_range((BYTE *)&dst_xs->YmmContext, single_range, 0, sizeof(dst_xs->YmmContext));


        *(DWORD *)((BYTE *)src + flags_offset) = arch_flags[i];

        src_xs->Mask = 7;
        memset(&dst_xs->YmmContext, 0xdd, sizeof(dst_xs->YmmContext));
        dst_xs->CompactionMask = 0xdddddddddddddddd;
        dst_xs->Mask = 0xdddddddddddddddd;
        status = pRtlCopyExtendedContext(dst_ex, flags, src_ex);
        ok(!status, "Got unexpected status %#x, flags %#x.\n", status, flags);
        ok(dst_xs->Mask == 4, "Got unexpected Mask %s.\n",
                wine_dbgstr_longlong(dst_xs->Mask));
        ok(dst_xs->CompactionMask == expected_compaction,
                "Got unexpected CompactionMask %s.\n", wine_dbgstr_longlong(dst_xs->CompactionMask));
        check_changes_in_range((BYTE *)&dst_xs->YmmContext, single_range, 1, sizeof(dst_xs->YmmContext));

        src_xs->Mask = 7;
        memset(&dst_xs->YmmContext, 0xdd, sizeof(dst_xs->YmmContext));
        dst_xs->CompactionMask = 0xdddddddddddddddd;
        dst_xs->Mask = 0xdddddddddddddddd;
        status = pRtlCopyContext(dst, flags, src);
        ok(!status || broken(!(flags & CONTEXT_NATIVE) && status == STATUS_INVALID_PARAMETER),
           "Got unexpected status %#x, flags %#x.\n", status, flags);
        ok(dst_xs->Mask == 0xdddddddddddddddd || broken(dst_xs->Mask == 4), "Got unexpected Mask %s, flags %#x.\n",
                wine_dbgstr_longlong(dst_xs->Mask), flags);
        ok(dst_xs->CompactionMask == 0xdddddddddddddddd || broken(dst_xs->CompactionMask == expected_compaction),
                "Got unexpected CompactionMask %s, flags %#x.\n", wine_dbgstr_longlong(dst_xs->CompactionMask), flags);
        check_changes_in_range((BYTE *)&dst_xs->YmmContext, single_range,
                dst_xs->Mask == 4, sizeof(dst_xs->YmmContext));
    }
}
#endif

START_TEST(exception)
{
    HMODULE hntdll = GetModuleHandleA("ntdll.dll");
    HMODULE hkernel32 = GetModuleHandleA("kernel32.dll");
#if defined(__x86_64__)
    HMODULE hmsvcrt = LoadLibraryA("msvcrt.dll");
#endif

    my_argc = winetest_get_mainargs( &my_argv );

    if (my_argc >= 3 && !strcmp(my_argv[2], "suspend_process"))
    {
        suspend_process_proc();
        return;
    }

    code_mem = VirtualAlloc(NULL, 65536, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if(!code_mem) {
        trace("VirtualAlloc failed\n");
        return;
    }

#define X(f) p##f = (void*)GetProcAddress(hntdll, #f)
    X(NtGetContextThread);
    X(NtSetContextThread);
    X(NtQueueApcThread);
    X(NtReadVirtualMemory);
    X(NtClose);
    X(RtlUnwind);
    X(RtlRaiseException);
    X(RtlCaptureContext);
    X(NtTerminateProcess);
    X(RtlAddVectoredExceptionHandler);
    X(RtlRemoveVectoredExceptionHandler);
    X(RtlAddVectoredContinueHandler);
    X(RtlRemoveVectoredContinueHandler);
    X(RtlSetUnhandledExceptionFilter);
    X(NtQueryInformationProcess);
    X(NtQueryInformationThread);
    X(NtSetInformationProcess);
    X(NtSuspendProcess);
    X(NtRaiseException);
    X(NtResumeProcess);
    X(RtlGetUnloadEventTrace);
    X(RtlGetUnloadEventTraceEx);
    X(RtlGetEnabledExtendedFeatures);
    X(RtlGetExtendedContextLength);
    X(RtlGetExtendedContextLength2);
    X(RtlInitializeExtendedContext);
    X(RtlInitializeExtendedContext2);
    X(RtlLocateExtendedFeature);
    X(RtlLocateLegacyContext);
    X(RtlSetExtendedFeaturesMask);
    X(RtlGetExtendedFeaturesMask);
    X(RtlCopyContext);
    X(RtlCopyExtendedContext);
#undef X

#define X(f) p##f = (void*)GetProcAddress(hkernel32, #f)
    X(IsWow64Process);
    if (!pIsWow64Process || !pIsWow64Process( GetCurrentProcess(), &is_wow64 )) is_wow64 = FALSE;

    X(InitializeContext);
    X(InitializeContext2);
    X(LocateXStateFeature);
    X(SetXStateFeaturesMask);
    X(GetXStateFeaturesMask);
#undef X

    if (pRtlAddVectoredExceptionHandler && pRtlRemoveVectoredExceptionHandler)
        have_vectored_api = TRUE;
    else
        skip("RtlAddVectoredExceptionHandler or RtlRemoveVectoredExceptionHandler not found\n");

    my_argc = winetest_get_mainargs( &my_argv );
    if (my_argc >= 4)
    {
        void *addr;
        sscanf( my_argv[3], "%p", &addr );

        if (addr != &test_stage)
        {
            skip( "child process not mapped at same address (%p/%p)\n", &test_stage, addr);
            return;
        }

        /* child must be run under a debugger */
        if (!NtCurrentTeb()->Peb->BeingDebugged)
        {
            ok(FALSE, "child process not being debugged?\n");
            return;
        }

#if defined(__i386__) || defined(__x86_64__)
        if (pRtlRaiseException)
        {
            test_stage = 1;
            run_rtlraiseexception_test(0x12345);
            run_rtlraiseexception_test(EXCEPTION_BREAKPOINT);
            run_rtlraiseexception_test(EXCEPTION_INVALID_HANDLE);
            test_stage = 2;
            run_rtlraiseexception_test(0x12345);
            run_rtlraiseexception_test(EXCEPTION_BREAKPOINT);
            run_rtlraiseexception_test(EXCEPTION_INVALID_HANDLE);
        }
        else skip( "RtlRaiseException not found\n" );
#endif
        test_stage = 3;
        test_outputdebugstring(0, FALSE);
        test_stage = 4;
        test_outputdebugstring(2, TRUE); /* is this a Windows bug? */
        test_stage = 5;
        test_ripevent(0);
        test_stage = 6;
        test_ripevent(1);
        test_stage = 7;
        test_debug_service(0);
        test_stage = 8;
        test_debug_service(1);
        test_stage = 9;
        test_breakpoint(0);
        test_stage = 10;
        test_breakpoint(1);
        test_stage = 11;
        test_closehandle(0, (HANDLE)0xdeadbeef);
        test_stage = 12;
        test_closehandle(1, (HANDLE)0xdeadbeef);
        test_stage = 13;
        test_closehandle(0, 0); /* Special case. */
#if defined(__i386__) || defined(__x86_64__)
        test_stage = 14;
        test_debuggee_xstate();
        test_stage = 15;
        test_debuggee_xstate();
        test_stage = 16;
        test_debuggee_segments();
#endif

        /* rest of tests only run in parent */
        return;
    }

#ifdef __i386__

    test_unwind();
    test_exceptions();
    test_rtlraiseexception();
    test_debug_registers();
    test_debug_service(1);
    test_simd_exceptions();
    test_fpu_exceptions();
    test_dpe_exceptions();
    test_prot_fault();
    test_kiuserexceptiondispatcher();
    test_extended_context();
    test_copy_context();

#elif defined(__x86_64__)

#define X(f) p##f = (void*)GetProcAddress(hntdll, #f)
    X(RtlAddFunctionTable);
    X(RtlDeleteFunctionTable);
    X(RtlInstallFunctionTableCallback);
    X(RtlLookupFunctionEntry);
    X(RtlAddGrowableFunctionTable);
    X(RtlGrowFunctionTable);
    X(RtlDeleteGrowableFunctionTable);
    X(__C_specific_handler);
    X(RtlCaptureContext);
    X(RtlRestoreContext);
    X(RtlUnwindEx);
    X(RtlWow64GetThreadContext);
    X(RtlWow64SetThreadContext);
    X(RtlWow64GetCpuAreaInfo);
#undef X
    p_setjmp = (void *)GetProcAddress( hmsvcrt, "_setjmp" );

    test_exceptions();
    test_rtlraiseexception();
    test_debug_registers();
    test_debug_registers_wow64();
    test_debug_service(1);
    test_simd_exceptions();
    test_virtual_unwind();
    test___C_specific_handler();
    test_restore_context();
    test_prot_fault();
    test_dpe_exceptions();
    test_wow64_context();
    test_kiuserexceptiondispatcher();
    test_nested_exception();

    if (pRtlAddFunctionTable && pRtlDeleteFunctionTable && pRtlInstallFunctionTableCallback && pRtlLookupFunctionEntry)
      test_dynamic_unwind();
    else
      skip( "Dynamic unwind functions not found\n" );
    test_extended_context();
    test_copy_context();
    test_unwind_from_apc();

#elif defined(__aarch64__)

    test_virtual_unwind();

#endif

    test_debugger(DBG_EXCEPTION_HANDLED);
    test_debugger(DBG_CONTINUE);
    test_thread_context();
    test_outputdebugstring(1, FALSE);
    test_ripevent(1);
    test_breakpoint(1);
    test_closehandle(0, (HANDLE)0xdeadbeef);
    /* Call of Duty WWII writes to BeingDebugged then closes an invalid handle,
     * crashing the game if an exception is raised. */
    NtCurrentTeb()->Peb->BeingDebugged = 0x98;
    test_closehandle(0, (HANDLE)0xdeadbeef);
    NtCurrentTeb()->Peb->BeingDebugged = 0;

    test_user_apc();
    test_vectored_continue_handler();
    test_suspend_thread();
    test_suspend_process();
    test_unload_trace();
    VirtualFree(code_mem, 0, MEM_RELEASE);
}
