/*
 * PowerPC64 (ELFv2) PE-side signal/exception support
 *
 * Copyright 1999, 2005 Alexandre Julliard
 * Copyright 2026 Jbettcher
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

#ifdef __powerpc64__

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <setjmp.h>

#include "ntstatus.h"
#include "windef.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "wine/exception.h"
#include "ntdll_misc.h"
#include "wine/debug.h"
#include "ntsyscalls.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);
WINE_DECLARE_DEBUG_CHANNEL(relay);

/* PowerPC64 has no Microsoft ABI, no PE unwind format and no .pdata producer.
 * On this port every "PE" module is in fact an ELF shared object built by
 * winegcc (PE_ARCHS is empty, DLLEXT is .so), so:
 *
 *   - RtlLookupFunctionEntry() always returns NULL and dispatch.LanguageHandler
 *     is therefore always NULL.  Structured exception handling in builtin
 *     modules runs entirely through the TEB exception-registration list that
 *     wine/exception.h's __TRY/__EXCEPT macros push, and those handlers resume
 *     with __wine_longjmp rather than by restoring a CONTEXT.
 *   - Stack walking uses the ELFv2 back chain (0(r1)) and the LR save slot
 *     (16(r1) of the caller's frame), which every ABI-conforming ppc64 function
 *     maintains.  See RtlVirtualUnwind2() in unwind.c.
 *
 * Consequences that are deliberately NOT implemented here, and are recorded as
 * such rather than papered over:
 *   - virtual_unwind() cannot restore non-volatile registers, so an unwind
 *     that resumes by RtlRestoreContext() will resume with the *unwinder's*
 *     r14-r31/f14-f31/v20-v31, not the target frame's.  That is correct for
 *     the __TRY/__EXCEPT + __wine_longjmp path (longjmp restores them from the
 *     jmp_buf) and wrong for a hypothetical PE __try/__except, of which there
 *     are none on this architecture.
 *   - Nested/collided unwind detection inside call_seh_handler() /
 *     call_unwind_handler() is absent: there is no .seh_handler equivalent in
 *     ELF asm and the TEB-frame hack would recurse.  Those two helpers are
 *     plain calls.
 */

/* CONTEXT field offsets used by the assembly below; kept honest by C_ASSERT. */
#define CTX_Fpr0         0x000
#define CTX_Fpscr        0x100
#define CTX_Gpr0         0x108
#define CTX_Gpr1         0x110
#define CTX_Gpr2         0x118
#define CTX_Gpr3         0x120
#define CTX_Cr           0x208
#define CTX_Xer          0x210
#define CTX_Msr          0x218
#define CTX_Iar          0x220
#define CTX_Lr           0x228
#define CTX_Ctr          0x230
#define CTX_ContextFlags 0x238
#define CTX_Vscr         0x298
#define CTX_Vrsave       0x29c
#define CTX_Vr0          0x2a0
#define CTX_SIZE         0x4a0

C_ASSERT( offsetof(CONTEXT,Fpr0)         == CTX_Fpr0 );
C_ASSERT( offsetof(CONTEXT,Fpscr)        == CTX_Fpscr );
C_ASSERT( offsetof(CONTEXT,Gpr0)         == CTX_Gpr0 );
C_ASSERT( offsetof(CONTEXT,Gpr1)         == CTX_Gpr1 );
C_ASSERT( offsetof(CONTEXT,Gpr2)         == CTX_Gpr2 );
C_ASSERT( offsetof(CONTEXT,Gpr3)         == CTX_Gpr3 );
C_ASSERT( offsetof(CONTEXT,Gpr31)        == CTX_Gpr0 + 31 * 8 );
C_ASSERT( offsetof(CONTEXT,Cr)           == CTX_Cr );
C_ASSERT( offsetof(CONTEXT,Xer)          == CTX_Xer );
C_ASSERT( offsetof(CONTEXT,Msr)          == CTX_Msr );
C_ASSERT( offsetof(CONTEXT,Iar)          == CTX_Iar );
C_ASSERT( offsetof(CONTEXT,Lr)           == CTX_Lr );
C_ASSERT( offsetof(CONTEXT,Ctr)          == CTX_Ctr );
C_ASSERT( offsetof(CONTEXT,ContextFlags) == CTX_ContextFlags );
C_ASSERT( offsetof(CONTEXT,Vscr)         == CTX_Vscr );
C_ASSERT( offsetof(CONTEXT,Vrsave)       == CTX_Vrsave );
C_ASSERT( offsetof(CONTEXT,Vr)           == CTX_Vr0 );
C_ASSERT( sizeof(CONTEXT)                == CTX_SIZE );

/* _JUMP_BUFFER must stay byte-identical to the __wine_jmp_buf that
 * libs/winecrt0/setjmp.c writes; both are hand-written assembly against literal
 * offsets, so pin every one of them here rather than trusting an eyeball match.
 * The numbers on the right are the offsets that appear in winecrt0's
 * __wine_setjmpex. */
C_ASSERT( offsetof(struct _JUMP_BUFFER,Frame) ==   0 );
C_ASSERT( offsetof(struct _JUMP_BUFFER,Gpr)   ==   8 );   /* r14-r31 */
C_ASSERT( offsetof(struct _JUMP_BUFFER,Sp)    == 152 );
C_ASSERT( offsetof(struct _JUMP_BUFFER,Toc)   == 160 );
C_ASSERT( offsetof(struct _JUMP_BUFFER,Cr)    == 168 );
C_ASSERT( offsetof(struct _JUMP_BUFFER,Lr)    == 176 );
C_ASSERT( offsetof(struct _JUMP_BUFFER,Fpr)   == 184 );   /* f14-f31 */
C_ASSERT( offsetof(struct _JUMP_BUFFER,Vr)    == 336 );   /* v20-v31 */
C_ASSERT( offsetof(struct _JUMP_BUFFER,Vr) % 16 == 0 );   /* stvx force-aligns */
C_ASSERT( sizeof(struct _JUMP_BUFFER)         <= sizeof(__wine_jmp_buf) );

/* CONTEXT_FULL, spelled out for the assembly that builds it with lis/ori. */
C_ASSERT( CONTEXT_FULL == 0x800017 );


/***********************************************************************
 *           NtCurrentTeb   (NTDLL.@)
 *
 * PowerPC64 has no register the Wine ABI can steal for the TEB: r13 is the ELF
 * thread pointer (glibc's), r2 is the TOC and r1 the stack pointer.  So the TEB
 * lives in an initial-exec thread-local of this module.  Measured on the AC922:
 * an initial-exec __thread in a dlopen()ed shared object resolves correctly and
 * compiles to "ld rN,off(r2); add rN,rN,r13" - no __tls_get_addr, no libc
 * dependency, which matters because ntdll.dll.so is linked -nodefaultlibs.
 *
 * The syscall thunks in include/wine/asm.h read this same variable directly,
 * with the ordinary @got@tprel/@tls relocations, so there is no cached offset
 * anywhere and nothing to initialise before the first syscall.  A cache was
 * tried and rejected: it would have to be filled by a constructor, and Wine
 * renames DT_INIT_ARRAY to a private dynamic tag in these modules so that it
 * can run constructors itself at DLL_PROCESS_ATTACH - far too late.  Measured:
 * a constructor added here was present in .init_array and still never ran.
 * An unfilled cache would have made every thunk read a "TEB" out of glibc's
 * TCB at 0(r13) and continue with garbage.
 */
__attribute__((visibility("hidden"))) __thread TEB *ppc64_current_teb
    __attribute__((tls_model("initial-exec")));

TEB * WINAPI NtCurrentTeb(void)
{
    return ppc64_current_teb;
}

/***********************************************************************
 *           __wine_init_teb
 *
 * Called by the unix library on every thread before any PE code runs, and
 * before the first syscall of that thread.  Not exported through the spec file;
 * the unix loader resolves it out of the PE ntdll export table by name.
 */
void CDECL __wine_init_teb( TEB *teb )
{
    ppc64_current_teb = teb;
}


/*******************************************************************
 *         syscall thunks
 *
 * __ASM_SYSCALL_FUNC (include/wine/asm.h) emits, per syscall:
 *
 *      <global entry>  addis r2,r12,.TOC.-name@ha ; addi r2,r2,.TOC.-name@l
 *      <local entry>   li    r11,<id>
 *                      b     __wine_syscall
 *
 * so on arrival at __wine_syscall: r2 = ntdll's TOC, r11 = syscall id, LR still
 * holds the PE caller's return address (the thunk branched, it did not call),
 * and r3-r10 plus the caller's parameter save area still hold the arguments.
 *
 * __wine_syscall adds r0 = TEB and tail-jumps to __wine_syscall_dispatcher with
 * r12 = the dispatcher address, which is what an ELFv2 global entry point
 * requires.  Handing the TEB over in r0 spares the unix-side dispatcher from
 * having to do TLS access in assembly; r0 is volatile and is not an argument
 * register, and r11/r12 are the only other registers free at this point.
 */
#define SYSCALL_ENTRY(id,name,args) __ASM_SYSCALL_FUNC( id, name )
ALL_SYSCALLS
#undef SYSCALL_ENTRY


/**************************************************************************
 *		__chkstk (NTDLL.@)
 *
 * Supposed to touch all the stack pages, but we shouldn't need that.
 */
__ASM_GLOBAL_FUNC( __chkstk, "blr" )


/***********************************************************************
 *		RtlCaptureContext (NTDLL.@)
 *
 * Deliberately TOC-free so that it neither needs nor destroys the caller's r2,
 * which it must record as Gpr2.  st_other stays 0, which is exactly the ELFv2
 * encoding for "single entry point, does not use r2".
 *
 * The vector stores use stvx, which force-aligns its effective address down to
 * a 16-byte boundary.  CONTEXT is DECLSPEC_ALIGN(16), so a properly declared
 * CONTEXT is always suitably aligned; a misaligned one would silently corrupt
 * memory below it.  Same contract as libs/winecrt0/setjmp.c.
 */
__ASM_GLOBAL_FUNC( RtlCaptureContext,
                   "stfd 0, 0x000(3)\n\t"
                   "stfd 1, 0x008(3)\n\t"
                   "stfd 2, 0x010(3)\n\t"
                   "stfd 3, 0x018(3)\n\t"
                   "stfd 4, 0x020(3)\n\t"
                   "stfd 5, 0x028(3)\n\t"
                   "stfd 6, 0x030(3)\n\t"
                   "stfd 7, 0x038(3)\n\t"
                   "stfd 8, 0x040(3)\n\t"
                   "stfd 9, 0x048(3)\n\t"
                   "stfd 10, 0x050(3)\n\t"
                   "stfd 11, 0x058(3)\n\t"
                   "stfd 12, 0x060(3)\n\t"
                   "stfd 13, 0x068(3)\n\t"
                   "stfd 14, 0x070(3)\n\t"
                   "stfd 15, 0x078(3)\n\t"
                   "stfd 16, 0x080(3)\n\t"
                   "stfd 17, 0x088(3)\n\t"
                   "stfd 18, 0x090(3)\n\t"
                   "stfd 19, 0x098(3)\n\t"
                   "stfd 20, 0x0a0(3)\n\t"
                   "stfd 21, 0x0a8(3)\n\t"
                   "stfd 22, 0x0b0(3)\n\t"
                   "stfd 23, 0x0b8(3)\n\t"
                   "stfd 24, 0x0c0(3)\n\t"
                   "stfd 25, 0x0c8(3)\n\t"
                   "stfd 26, 0x0d0(3)\n\t"
                   "stfd 27, 0x0d8(3)\n\t"
                   "stfd 28, 0x0e0(3)\n\t"
                   "stfd 29, 0x0e8(3)\n\t"
                   "stfd 30, 0x0f0(3)\n\t"
                   "stfd 31, 0x0f8(3)\n\t"
                   "mffs 0\n\t"                  /* f0 is already saved */
                   "stfd 0, 0x100(3)\n\t"        /* Fpscr; mffs writes f0, r0 is intact */
                   "std 0, 0x108(3)\n\t"         /* Gpr0, still the caller's value */
                   "std 1, 0x110(3)\n\t"         /* Gpr1: caller's sp, we push nothing */
                   "std 2, 0x118(3)\n\t"         /* Gpr2: caller's TOC */
                   "std 3, 0x120(3)\n\t"
                   "std 4, 0x128(3)\n\t"
                   "std 5, 0x130(3)\n\t"
                   "std 6, 0x138(3)\n\t"
                   "std 7, 0x140(3)\n\t"
                   "std 8, 0x148(3)\n\t"
                   "std 9, 0x150(3)\n\t"
                   "std 10, 0x158(3)\n\t"
                   "std 11, 0x160(3)\n\t"
                   "std 12, 0x168(3)\n\t"
                   "std 13, 0x170(3)\n\t"
                   "std 14, 0x178(3)\n\t"
                   "std 15, 0x180(3)\n\t"
                   "std 16, 0x188(3)\n\t"
                   "std 17, 0x190(3)\n\t"
                   "std 18, 0x198(3)\n\t"
                   "std 19, 0x1a0(3)\n\t"
                   "std 20, 0x1a8(3)\n\t"
                   "std 21, 0x1b0(3)\n\t"
                   "std 22, 0x1b8(3)\n\t"
                   "std 23, 0x1c0(3)\n\t"
                   "std 24, 0x1c8(3)\n\t"
                   "std 25, 0x1d0(3)\n\t"
                   "std 26, 0x1d8(3)\n\t"
                   "std 27, 0x1e0(3)\n\t"
                   "std 28, 0x1e8(3)\n\t"
                   "std 29, 0x1f0(3)\n\t"
                   "std 30, 0x1f8(3)\n\t"
                   "std 31, 0x200(3)\n\t"
                   "mfcr 0\n\t"
                   "std 0, 0x208(3)\n\t"         /* Cr */
                   "mfxer 0\n\t"
                   "std 0, 0x210(3)\n\t"         /* Xer */
                   "li 0, 0\n\t"
                   "std 0, 0x218(3)\n\t"         /* Msr: not readable in problem state */
                   "std 0, 0x240(3)\n\t"         /* Dar */
                   "std 0, 0x248(3)\n\t"         /* Dsisr */
                   "std 0, 0x250(3)\n\t"         /* Trap */
                   "stw 0, 0x298(3)\n\t"         /* Vscr: see comment below */
                   "stw 0, 0x29c(3)\n\t"         /* Vrsave */
                   "mflr 0\n\t"
                   "std 0, 0x220(3)\n\t"         /* Iar = our return address */
                   "std 0, 0x228(3)\n\t"         /* Lr */
                   "mfctr 0\n\t"
                   "std 0, 0x230(3)\n\t"         /* Ctr */
                   "li 0, 0x2a0\n\t"
                   "stvx 0, 3, 0\n\t"  "addi 0, 0, 16\n\t"
                   "stvx 1, 3, 0\n\t"  "addi 0, 0, 16\n\t"
                   "stvx 2, 3, 0\n\t"  "addi 0, 0, 16\n\t"
                   "stvx 3, 3, 0\n\t"  "addi 0, 0, 16\n\t"
                   "stvx 4, 3, 0\n\t"  "addi 0, 0, 16\n\t"
                   "stvx 5, 3, 0\n\t"  "addi 0, 0, 16\n\t"
                   "stvx 6, 3, 0\n\t"  "addi 0, 0, 16\n\t"
                   "stvx 7, 3, 0\n\t"  "addi 0, 0, 16\n\t"
                   "stvx 8, 3, 0\n\t"  "addi 0, 0, 16\n\t"
                   "stvx 9, 3, 0\n\t"  "addi 0, 0, 16\n\t"
                   "stvx 10, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 11, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 12, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 13, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 14, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 15, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 16, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 17, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 18, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 19, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 20, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 21, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 22, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 23, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 24, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 25, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 26, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 27, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 28, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 29, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 30, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 31, 3, 0\n\t"
                   "lis 0, 0x80\n\t"             /* CONTEXT_FULL = 0x00800017 */
                   "ori 0, 0, 0x17\n\t"
                   "std 0, 0x238(3)\n\t"
                   "blr" )

/* Vscr and Vrsave are stored as zero rather than captured: mfvscr delivers VSCR
 * inside a vector register whose element order differs between BE and LE, and
 * getting that wrong writes a plausible-looking wrong value.  Zero is the
 * architectural reset state (NJ=0, SAT=0).  Recorded as a stub. */


/**********************************************************************
 *           virtual_unwind
 */
static NTSTATUS virtual_unwind( ULONG type, DISPATCHER_CONTEXT *dispatch, CONTEXT *context )
{
    DWORD64 pc = context->Iar;

    dispatch->ScopeIndex = 0;
    dispatch->ControlPc  = pc;
    dispatch->ControlPcIsUnwound = (context->ContextFlags & CONTEXT_UNWOUND_TO_CALL) != 0;
    if (dispatch->ControlPcIsUnwound) pc -= 4;

    dispatch->FunctionEntry = RtlLookupFunctionEntry( pc, &dispatch->ImageBase, dispatch->HistoryTable );

    if (RtlVirtualUnwind2( type, dispatch->ImageBase, pc, dispatch->FunctionEntry, context,
                           NULL, &dispatch->HandlerData, &dispatch->EstablisherFrame,
                           NULL, NULL, NULL, &dispatch->LanguageHandler, 0 ))
    {
        WARN( "exception data not found for pc %p, lr %p\n", (void *)pc, (void *)context->Lr );
        return STATUS_INVALID_DISPOSITION;
    }
    return STATUS_SUCCESS;
}


/*******************************************************************
 *         nested_exception_handler
 */
EXCEPTION_DISPOSITION WINAPI nested_exception_handler( EXCEPTION_RECORD *rec, void *frame,
                                                       CONTEXT *context, void *dispatch )
{
    if (rec->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND)) return ExceptionContinueSearch;
    return ExceptionNestedException;
}


/***********************************************************************
 *		call_seh_handler / call_unwind_handler
 *
 * STUB, and knowingly so: on the other architectures these are assembly
 * trampolines carrying a .seh_handler, so that an exception raised *inside* a
 * handler is reported as ExceptionNestedException / ExceptionCollidedUnwind.
 * ELF ppc64 has no .seh_handler, and pushing a TEB frame here would be seen by
 * call_seh_handlers()'s own TEB-frame walk.  Nested and collided unwinds are
 * therefore not detected on this architecture.
 */
static DWORD call_seh_handler( EXCEPTION_RECORD *rec, ULONG_PTR frame,
                               CONTEXT *context, void *dispatch, PEXCEPTION_ROUTINE handler )
{
    return handler( rec, (void *)frame, context, dispatch );
}

static DWORD call_unwind_handler( EXCEPTION_RECORD *rec, ULONG_PTR frame,
                                  CONTEXT *context, void *dispatch, PEXCEPTION_ROUTINE handler )
{
    return handler( rec, (void *)frame, context, dispatch );
}


/**********************************************************************
 *           call_seh_handlers
 *
 * Call the SEH handlers.
 */
NTSTATUS call_seh_handlers( EXCEPTION_RECORD *rec, CONTEXT *orig_context )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;
    UNWIND_HISTORY_TABLE table;
    DISPATCHER_CONTEXT dispatch;
    CONTEXT context;
    NTSTATUS status;
    ULONG_PTR frame;
    DWORD res;

    context = *orig_context;
    dispatch.TargetPc      = 0;
    dispatch.ContextRecord = &context;
    dispatch.HistoryTable  = &table;
    dispatch.NonVolatileRegisters = NULL;

    for (;;)
    {
        status = virtual_unwind( UNW_FLAG_EHANDLER, &dispatch, &context );
        if (status != STATUS_SUCCESS) return status;

    unwind_done:
        if (!dispatch.EstablisherFrame) break;

        if (!is_valid_frame( dispatch.EstablisherFrame ))
        {
            ERR( "invalid frame %I64x (%p-%p)\n", dispatch.EstablisherFrame,
                 NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
            rec->ExceptionFlags |= EXCEPTION_STACK_INVALID;
            break;
        }

        if (dispatch.LanguageHandler)
        {
            TRACE( "calling handler %p (rec=%p, frame=%I64x context=%p, dispatch=%p)\n",
                   dispatch.LanguageHandler, rec, dispatch.EstablisherFrame, orig_context, &dispatch );
            res = call_seh_handler( rec, dispatch.EstablisherFrame, orig_context,
                                    &dispatch, dispatch.LanguageHandler );
            rec->ExceptionFlags &= EXCEPTION_NONCONTINUABLE;
            TRACE( "handler at %p returned %lu\n", dispatch.LanguageHandler, res );

            switch (res)
            {
            case ExceptionContinueExecution:
                if (rec->ExceptionFlags & EXCEPTION_NONCONTINUABLE) return STATUS_NONCONTINUABLE_EXCEPTION;
                return STATUS_SUCCESS;
            case ExceptionContinueSearch:
                break;
            case ExceptionNestedException:
                rec->ExceptionFlags |= EXCEPTION_NESTED_CALL;
                TRACE( "nested exception\n" );
                break;
            case ExceptionCollidedUnwind:
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                                  dispatch.ControlPc, dispatch.FunctionEntry,
                                  &context, &dispatch.HandlerData, &frame, NULL );
                goto unwind_done;
            default:
                return STATUS_INVALID_DISPOSITION;
            }
        }
        /* hack: call wine handlers registered in the tib list */
        else while (is_valid_frame( (ULONG_PTR)teb_frame ) && (ULONG64)teb_frame < context.Gpr1)
        {
            TRACE( "calling TEB handler %p (rec=%p frame=%p context=%p dispatch=%p) sp=%I64x\n",
                   teb_frame->Handler, rec, teb_frame, orig_context, &dispatch, context.Gpr1 );
            res = call_seh_handler( rec, (ULONG_PTR)teb_frame, orig_context,
                                    &dispatch, (PEXCEPTION_ROUTINE)teb_frame->Handler );
            TRACE( "TEB handler at %p returned %lu\n", teb_frame->Handler, res );

            switch (res)
            {
            case ExceptionContinueExecution:
                if (rec->ExceptionFlags & EXCEPTION_NONCONTINUABLE) return STATUS_NONCONTINUABLE_EXCEPTION;
                return STATUS_SUCCESS;
            case ExceptionContinueSearch:
                break;
            case ExceptionNestedException:
                rec->ExceptionFlags |= EXCEPTION_NESTED_CALL;
                TRACE( "nested exception\n" );
                break;
            case ExceptionCollidedUnwind:
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                                  dispatch.ControlPc, dispatch.FunctionEntry,
                                  &context, &dispatch.HandlerData, &frame, NULL );
                teb_frame = teb_frame->Prev;
                goto unwind_done;
            default:
                return STATUS_INVALID_DISPOSITION;
            }
            teb_frame = teb_frame->Prev;
        }

        if (context.Gpr1 == (ULONG64)NtCurrentTeb()->Tib.StackBase) break;
    }
    return STATUS_UNHANDLED_EXCEPTION;
}


/*******************************************************************
 *		KiUserExceptionDispatcher (NTDLL.@)
 *
 * Entered by a jump from the unix side, NOT by a call.  Contract (ppc64 only,
 * there is no Windows ABI to match):
 *      r3  = EXCEPTION_RECORD *      (on the user stack)
 *      r4  = CONTEXT *               (on the user stack, 16-byte aligned)
 *      r12 = KiUserExceptionDispatcher, so the ELFv2 global entry prologue that
 *            the compiler emits for this C function computes the right TOC
 *      r1  = a 16-byte-aligned user stack pointer with a valid back chain,
 *            below the record and context
 * LR is undefined on entry; this function never returns.
 */
void WINAPI KiUserExceptionDispatcher( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    NTSTATUS status = dispatch_exception( rec, context );
    RtlRaiseStatus( status );
}


/*******************************************************************
 *		KiUserApcDispatcher (NTDLL.@)
 *
 * Entered by a jump from the unix side with the arguments in r3-r8 and r12 =
 * KiUserApcDispatcher; see KiUserExceptionDispatcher for the rationale.
 */
void WINAPI KiUserApcDispatcher( CONTEXT *context, ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3,
                                 PNTAPCFUNC func )
{
    func( arg1, arg2, arg3 );
    NtContinue( context, TRUE );
    RtlRaiseStatus( STATUS_ACCESS_VIOLATION );
}


/*******************************************************************
 *		KiUserCallbackDispatcher (NTDLL.@)
 */
void WINAPI KiUserCallbackDispatcher( ULONG id, void *args, ULONG len )
{
    NTSTATUS status = dispatch_user_callback( args, len, id );

    status = NtCallbackReturn( NULL, 0, status );
    RtlRaiseStatus( status );
}


/*******************************************************************
 *              RtlRestoreContext (NTDLL.@)
 */
void CDECL RtlRestoreContext( CONTEXT *context, EXCEPTION_RECORD *rec )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;

    if (rec && rec->ExceptionCode == STATUS_LONGJUMP && rec->NumberParameters >= 1)
    {
        struct _JUMP_BUFFER *jmp = (struct _JUMP_BUFFER *)rec->ExceptionInformation[0];
        int i;

        for (i = 0; i < 18; i++) (&context->Gpr14)[i] = jmp->Gpr[i];
        for (i = 0; i < 18; i++) (&context->Fpr14)[i] = jmp->Fpr[i];
        for (i = 0; i < 12; i++)
        {
            context->Vr[20 + i].Low  = jmp->Vr[2 * i];
            context->Vr[20 + i].High = jmp->Vr[2 * i + 1];
        }
        context->Gpr1 = jmp->Sp;
        context->Gpr2 = jmp->Toc;
        context->Cr   = jmp->Cr;
        context->Lr   = jmp->Lr;
        context->Iar  = jmp->Lr;
    }
    else if (rec && rec->ExceptionCode == STATUS_UNWIND_CONSOLIDATE && rec->NumberParameters >= 1)
    {
        PVOID (CALLBACK *consolidate)(EXCEPTION_RECORD *) = (void *)rec->ExceptionInformation[0];
        TRACE( "calling consolidate callback %p (rec=%p)\n", consolidate, rec );
        /* STUB: the other architectures run the callback on a synthesised frame
         * (consolidate_callback/invoke_callback) so that an RtlUnwindEx issued
         * from inside it - which C++ handlers do - skips the frames already
         * processed.  That trick needs a resumable CONTEXT, and the back-chain
         * unwinder cannot produce one.  Called directly instead: a nested
         * unwind from the callback will re-walk frames it has already unwound. */
        context->Iar = (ULONG64)consolidate( rec );
        context->Lr  = context->Iar;
        context->Gpr12 = context->Iar;   /* see RtlUnwindEx: ELFv2 global entry needs r12 */
    }

    /* hack: remove no longer accessible TEB frames */
    while (is_valid_frame( (ULONG_PTR)teb_frame ) && (ULONG64)teb_frame < context->Gpr1)
    {
        TRACE( "removing TEB frame: %p\n", teb_frame );
        teb_frame = __wine_pop_frame( teb_frame );
    }

    TRACE( "returning to %I64x stack %I64x\n", context->Iar, context->Gpr1 );
    NtContinue( context, FALSE );
}


/*******************************************************************
 *		RtlUnwindEx (NTDLL.@)
 */
void WINAPI RtlUnwindEx( PVOID end_frame, PVOID target_ip, EXCEPTION_RECORD *rec,
                         PVOID retval, CONTEXT *context, UNWIND_HISTORY_TABLE *table )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;
    EXCEPTION_RECORD record;
    DISPATCHER_CONTEXT dispatch;
    CONTEXT new_context;
    NTSTATUS status;
    ULONG_PTR frame;
    DWORD i, res;

    RtlCaptureContext( context );
    new_context = *context;

    /* build an exception record, if we do not have one */
    if (!rec)
    {
        record.ExceptionCode    = STATUS_UNWIND;
        record.ExceptionFlags   = 0;
        record.ExceptionRecord  = NULL;
        record.ExceptionAddress = (void *)context->Iar;
        record.NumberParameters = 0;
        rec = &record;
    }

    rec->ExceptionFlags |= EXCEPTION_UNWINDING | (end_frame ? 0 : EXCEPTION_EXIT_UNWIND);

    TRACE( "code=%lx flags=%lx end_frame=%p target_ip=%p\n",
           rec->ExceptionCode, rec->ExceptionFlags, end_frame, target_ip );
    for (i = 0; i < min( EXCEPTION_MAXIMUM_PARAMETERS, rec->NumberParameters ); i++)
        TRACE( " info[%ld]=%016I64x\n", i, rec->ExceptionInformation[i] );
    TRACE_CONTEXT( context );

    dispatch.TargetPc         = (ULONG64)target_ip;
    dispatch.ContextRecord    = context;
    dispatch.HistoryTable     = table;
    dispatch.NonVolatileRegisters = NULL;

    for (;;)
    {
        status = virtual_unwind( UNW_FLAG_UHANDLER, &dispatch, &new_context );
        if (status != STATUS_SUCCESS) raise_status( status, rec );

    unwind_done:
        if (!dispatch.EstablisherFrame) break;

        if (!is_valid_frame( dispatch.EstablisherFrame ))
        {
            ERR( "invalid frame %I64x (%p-%p)\n", dispatch.EstablisherFrame,
                 NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
            rec->ExceptionFlags |= EXCEPTION_STACK_INVALID;
            break;
        }

        if (dispatch.LanguageHandler)
        {
            if (end_frame && (dispatch.EstablisherFrame > (ULONG64)end_frame))
            {
                ERR( "invalid end frame %I64x/%p\n", dispatch.EstablisherFrame, end_frame );
                raise_status( STATUS_INVALID_UNWIND_TARGET, rec );
            }
            if (dispatch.EstablisherFrame == (ULONG64)end_frame) rec->ExceptionFlags |= EXCEPTION_TARGET_UNWIND;

            res = call_unwind_handler( rec, dispatch.EstablisherFrame, dispatch.ContextRecord,
                                       &dispatch, dispatch.LanguageHandler );
            TRACE( "handler %p returned %lx\n", dispatch.LanguageHandler, res );

            switch (res)
            {
            case ExceptionContinueSearch:
                rec->ExceptionFlags &= ~EXCEPTION_COLLIDED_UNWIND;
                break;
            case ExceptionCollidedUnwind:
                new_context = *context;
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                                  dispatch.ControlPc, dispatch.FunctionEntry,
                                  &new_context, &dispatch.HandlerData, &frame, NULL );
                rec->ExceptionFlags |= EXCEPTION_COLLIDED_UNWIND;
                goto unwind_done;
            default:
                raise_status( STATUS_INVALID_DISPOSITION, rec );
                break;
            }
        }
        else  /* hack: call builtin handlers registered in the tib list */
        {
            ULONG_PTR last_frame = new_context.Gpr1;
            if (end_frame && (ULONG_PTR)end_frame < last_frame) last_frame = (ULONG_PTR)end_frame;

            while (is_valid_frame( (ULONG_PTR)teb_frame ) && (ULONG_PTR)teb_frame < last_frame)
            {
                TRACE( "calling TEB handler %p (rec=%p, frame=%p context=%p, dispatch=%p)\n",
                       teb_frame->Handler, rec, teb_frame, dispatch.ContextRecord, &dispatch );
                res = call_unwind_handler( rec, (ULONG_PTR)teb_frame, dispatch.ContextRecord, &dispatch,
                                           (PEXCEPTION_ROUTINE)teb_frame->Handler );
                TRACE( "handler at %p returned %lu\n", teb_frame->Handler, res );
                teb_frame = __wine_pop_frame( teb_frame );

                switch (res)
                {
                case ExceptionContinueSearch:
                    rec->ExceptionFlags &= ~EXCEPTION_COLLIDED_UNWIND;
                    break;
                case ExceptionCollidedUnwind:
                    new_context = *context;
                    RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                                      dispatch.ControlPc, dispatch.FunctionEntry,
                                      &new_context, &dispatch.HandlerData,
                                      &frame, NULL );
                    rec->ExceptionFlags |= EXCEPTION_COLLIDED_UNWIND;
                    goto unwind_done;
                default:
                    raise_status( STATUS_INVALID_DISPOSITION, rec );
                    break;
                }
            }
            if ((ULONG_PTR)teb_frame == last_frame && last_frame < new_context.Gpr1) break;
        }

        if (dispatch.EstablisherFrame == (ULONG64)end_frame) break;
        *context = new_context;
    }

    if (rec->ExceptionCode != STATUS_UNWIND_CONSOLIDATE)
    {
        context->Iar = (ULONG64)target_ip;
        context->Lr  = (ULONG64)target_ip;
        /* An ELFv2 global entry point begins "addis r2,r12,.TOC.-f@ha; addi
         * r2,r2,.TOC.-f@l", so resuming at the entry of a function reached
         * through a function pointer - which is what winecrt0's unwind_target
         * is - only computes the right TOC if r12 holds that entry address.
         * Nothing else on this path sets it: virtual_unwind() walks the back
         * chain and recovers Iar, Lr and Gpr1 only, so Gpr12 would otherwise
         * still be RtlUnwindEx's own.  Measured: leaving it produced a garbage
         * r2, a garbage indirect branch out of unwind_target, and an endless
         * fault loop during kernel32's PROCESS_ATTACH.
         * r12 is volatile, so writing it is harmless when target_ip is an
         * ordinary label inside a function rather than an entry point. */
        context->Gpr12 = (ULONG64)target_ip;
    }

    context->Gpr3 = (ULONG64)retval;
    RtlRestoreContext( context, rec );
}


/*************************************************************************
 *		RtlGetNativeSystemInformation (NTDLL.@)
 */
NTSTATUS WINAPI RtlGetNativeSystemInformation( SYSTEM_INFORMATION_CLASS class,
                                               void *info, ULONG size, ULONG *ret_size )
{
    return NtQuerySystemInformation( class, info, size, ret_size );
}


/***********************************************************************
 *           RtlIsProcessorFeaturePresent [NTDLL.@]
 *
 * The PF_* feature numbers are all x86/ARM specific; none of them describes a
 * PowerPC capability, so nothing is ever present.  Stub, deliberately.
 */
BOOLEAN WINAPI RtlIsProcessorFeaturePresent( UINT feature )
{
    return FALSE;
}


/*************************************************************************
 *		RtlWalkFrameChain (NTDLL.@)
 */
ULONG WINAPI RtlWalkFrameChain( void **buffer, ULONG count, ULONG flags )
{
    UNWIND_HISTORY_TABLE table;
    RUNTIME_FUNCTION *func;
    PEXCEPTION_ROUTINE handler;
    ULONG_PTR pc, frame, base;
    CONTEXT context;
    void *data;
    ULONG i, skip = flags >> 8, num_entries = 0;

    RtlCaptureContext( &context );

    for (i = 0; i < count; i++)
    {
        pc = context.Iar;
        if (context.ContextFlags & CONTEXT_UNWOUND_TO_CALL) pc -= 4;
        func = RtlLookupFunctionEntry( pc, &base, &table );
        if (RtlVirtualUnwind2( UNW_FLAG_NHANDLER, base, pc, func, &context, NULL,
                               &data, &frame, NULL, NULL, NULL, &handler, 0 ))
            break;
        if (!context.Iar) break;
        if (!frame || !is_valid_frame( frame )) break;
        if (context.Gpr1 == (ULONG_PTR)NtCurrentTeb()->Tib.StackBase) break;
        if (i >= skip) buffer[num_entries++] = (void *)context.Iar;
    }
    return num_entries;
}


/***********************************************************************
 *		__C_ExecuteExceptionFilter
 *
 * Only reachable from __C_specific_handler, which only ever runs against PE
 * unwind data.  There is none on this architecture, so this cannot be called;
 * it exists so that unwind.c links.  Non-volatile registers are NOT reloaded
 * from the DISPATCHER_CONTEXT block (r6), because nothing populates it.
 */
LONG WINAPI __C_ExecuteExceptionFilter( void *record, void *frame,
                                        PEXCEPTION_FILTER filter, BYTE *nonvolatile )
{
    ERR( "not supported on PowerPC64\n" );
    return EXCEPTION_CONTINUE_SEARCH;
}


/***********************************************************************
 *		RtlRaiseException (NTDLL.@)
 *
 * Assembly wrapper: capture the caller's context, point it at the call site
 * rather than at RtlCaptureContext, and hand it to raise_exception_from_asm().
 * The 0x4c0-byte frame is CONTEXT (0x4a0, 16-byte aligned at offset 0x20 from
 * the new r1) plus the 32-byte ELFv2 linkage area.
 */
extern void DECLSPEC_NORETURN raise_exception_from_asm( EXCEPTION_RECORD *rec, CONTEXT *context );
__ASM_GLOBAL_FUNC( RtlRaiseException,
                   "addis 2, 12, .TOC.-" __ASM_NAME("RtlRaiseException") "@ha\n\t"
                   "addi 2, 2, .TOC.-" __ASM_NAME("RtlRaiseException") "@l\n\t"
                   ".localentry " __ASM_NAME("RtlRaiseException") ", .-" __ASM_NAME("RtlRaiseException") "\n\t"
                   "mflr 0\n\t"
                   "std 0, 16(1)\n\t"
                   "std 3, -8(1)\n\t"            /* stash rec in the red zone */
                   "stdu 1, -0x4e0(1)\n\t"
                   "addi 3, 1, 0x20\n\t"         /* &context */
                   "bl " __ASM_NAME("RtlCaptureContext") "\n\t"
                   "addi 4, 1, 0x20\n\t"         /* context */
                   "ld 3, 0x4d8(1)\n\t"          /* rec, from the red-zone stash */
                   "addi 5, 1, 0x4e0\n\t"
                   "std 5, 0x110(4)\n\t"         /* context->Gpr1 = caller's sp */
                   "std 3, 0x120(4)\n\t"         /* context->Gpr3 = rec */
                   "ld 0, 0x4f0(1)\n\t"          /* caller's LR save slot */
                   "std 0, 0x220(4)\n\t"         /* context->Iar */
                   "std 0, 0x228(4)\n\t"         /* context->Lr */
                   "std 0, 0x10(3)\n\t"          /* rec->ExceptionAddress */
                   "ld 0, 0x238(4)\n\t"
                   "oris 0, 0, 0x2000\n\t"       /* CONTEXT_UNWOUND_TO_CALL */
                   "std 0, 0x238(4)\n\t"
                   "b " __ASM_NAME("raise_exception_from_asm") )

void DECLSPEC_NORETURN raise_exception_from_asm( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    NTSTATUS status;

    if (!NtCurrentTeb()->Peb->BeingDebugged) status = dispatch_exception( rec, context );
    else status = NtRaiseException( rec, context, TRUE );
    RtlRaiseStatus( status );
}


/***********************************************************************
 *           _setjmpex (NTDLL.@)
 *
 * Layout must stay identical to libs/winecrt0/setjmp.c and to the _JUMP_BUFFER
 * in include/msvcrt/setjmp.h.
 */
__ASM_GLOBAL_FUNC( NTDLL__setjmpex,
                   "std  4, 0(3)\n\t"       /* Frame */
                   "std 14, 8(3)\n\t"
                   "std 15, 16(3)\n\t"
                   "std 16, 24(3)\n\t"
                   "std 17, 32(3)\n\t"
                   "std 18, 40(3)\n\t"
                   "std 19, 48(3)\n\t"
                   "std 20, 56(3)\n\t"
                   "std 21, 64(3)\n\t"
                   "std 22, 72(3)\n\t"
                   "std 23, 80(3)\n\t"
                   "std 24, 88(3)\n\t"
                   "std 25, 96(3)\n\t"
                   "std 26, 104(3)\n\t"
                   "std 27, 112(3)\n\t"
                   "std 28, 120(3)\n\t"
                   "std 29, 128(3)\n\t"
                   "std 30, 136(3)\n\t"
                   "std 31, 144(3)\n\t"
                   "std  1, 152(3)\n\t"     /* Sp */
                   "std  2, 160(3)\n\t"     /* Toc */
                   "mfcr 0\n\t"
                   "std  0, 168(3)\n\t"     /* Cr */
                   "mflr 0\n\t"
                   "std  0, 176(3)\n\t"     /* Lr */
                   "stfd 14, 184(3)\n\t"
                   "stfd 15, 192(3)\n\t"
                   "stfd 16, 200(3)\n\t"
                   "stfd 17, 208(3)\n\t"
                   "stfd 18, 216(3)\n\t"
                   "stfd 19, 224(3)\n\t"
                   "stfd 20, 232(3)\n\t"
                   "stfd 21, 240(3)\n\t"
                   "stfd 22, 248(3)\n\t"
                   "stfd 23, 256(3)\n\t"
                   "stfd 24, 264(3)\n\t"
                   "stfd 25, 272(3)\n\t"
                   "stfd 26, 280(3)\n\t"
                   "stfd 27, 288(3)\n\t"
                   "stfd 28, 296(3)\n\t"
                   "stfd 29, 304(3)\n\t"
                   "stfd 30, 312(3)\n\t"
                   "stfd 31, 320(3)\n\t"
                   "li   0, 336\n\t"
                   "stvx 20, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 21, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 22, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 23, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 24, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 25, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 26, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 27, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 28, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 29, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 30, 3, 0\n\t" "addi 0, 0, 16\n\t"
                   "stvx 31, 3, 0\n\t"
                   "li 3, 0\n\t"
                   "blr" )


/*******************************************************************
 *		longjmp (NTDLL.@)
 */
void __cdecl NTDLL_longjmp( _JUMP_BUFFER *buf, int retval )
{
    EXCEPTION_RECORD rec;

    if (!retval) retval = 1;

    rec.ExceptionCode = STATUS_LONGJUMP;
    rec.ExceptionFlags = 0;
    rec.ExceptionRecord = NULL;
    rec.ExceptionAddress = NULL;
    rec.NumberParameters = 1;
    rec.ExceptionInformation[0] = (DWORD_PTR)buf;
    RtlUnwind( (void *)buf->Frame, (void *)buf->Lr, &rec, IntToPtr(retval) );
}


/***********************************************************************
 *           RtlUserThreadStart (NTDLL.@)
 */
void WINAPI RtlUserThreadStart( PRTL_THREAD_START_ROUTINE entry, void *arg )
{
    __TRY
    {
        pBaseThreadInitThunk( 0, (LPTHREAD_START_ROUTINE)entry, arg );
    }
    __EXCEPT( call_unhandled_exception_filter )
    {
        NtTerminateProcess( GetCurrentProcess(), GetExceptionCode() );
    }
    __ENDTRY
}


/******************************************************************
 *		LdrInitializeThunk (NTDLL.@)
 */
void WINAPI LdrInitializeThunk( CONTEXT *context, ULONG_PTR unk2, ULONG_PTR unk3, ULONG_PTR unk4 )
{
    loader_init( context, (void **)&context->Gpr3 );
    TRACE_(relay)( "\1Starting thread proc %p (arg=%p)\n", (void *)context->Gpr3, (void *)context->Gpr4 );
    NtContinue( context, TRUE );
}


/***********************************************************************
 *           process_breakpoint
 *
 * "trap" raises SIGTRAP, which the unix side turns into a
 * STATUS_BREAKPOINT.  If a debugger is not attached the exception is
 * unhandled, so unlike the other architectures - which install an SEH handler
 * that steps over the trap - this deliberately only traps when a debugger is
 * present.  There is no .seh_handler on ELF ppc64 to do it the other way.
 */
void WINAPI process_breakpoint(void)
{
    if (NtCurrentTeb()->Peb->BeingDebugged) DbgBreakPoint();
}


/***********************************************************************
 *		DbgUiRemoteBreakin   (NTDLL.@)
 */
void WINAPI DbgUiRemoteBreakin( void *arg )
{
    __TRY
    {
        if (NtCurrentTeb()->Peb->BeingDebugged) DbgBreakPoint();
    }
    __EXCEPT_ALL
    {
        /* ignore */
    }
    __ENDTRY
    RtlExitUserThread( STATUS_SUCCESS );
}


/**********************************************************************
 *              DbgBreakPoint   (NTDLL.@)
 *
 * Padded with nops so that a debugger can patch the entry point, as on the
 * other architectures.
 */
__ASM_GLOBAL_FUNC( DbgBreakPoint, "trap\n\tblr\n\t"
                   "nop; nop; nop; nop; nop; nop; nop; nop\n\t"
                   "nop; nop; nop; nop; nop; nop" )

/**********************************************************************
 *              DbgUserBreakPoint   (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( DbgUserBreakPoint, "trap\n\tblr\n\t"
                   "nop; nop; nop; nop; nop; nop; nop; nop\n\t"
                   "nop; nop; nop; nop; nop; nop" )

#endif  /* __powerpc64__ */
