/*
 * File cpu_ppc64.c
 *
 * Copyright (C) 2009 Eric Pouech
 * Copyright (C) 2010-2013 André Hentschel
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

#include <assert.h>

#include "ntstatus.h"
#include "dbghelp_private.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

/* The DWARF register numbering for 64-bit PowerPC (as emitted by gcc and clang):
 *      0-31    r0-r31
 *     32-63    f0-f31
 *        65    lr          (this is also the return address column)
 *        66    ctr
 *     68-75    cr0-cr7
 *        76    xer
 *    77-108    v0-v31
 *       109    vrsave
 *       110    vscr
 * Register 64 is the (32-bit only, long obsolete) MQ register and is never
 * emitted here; it is deliberately not mapped.
 */
#define PPC64_DWARF_LR      65
#define PPC64_DWARF_CTR     66
#define PPC64_DWARF_CR0     68
#define PPC64_DWARF_XER     76

static BOOL ppc64_get_addr(HANDLE hThread, const CONTEXT* ctx,
                           enum cpu_addr ca, ADDRESS64* addr)
{
    addr->Mode    = AddrModeFlat;
    addr->Segment = 0; /* don't need segment */
    switch (ca)
    {
#ifdef __powerpc64__
    case cpu_addr_pc:    addr->Offset = ctx->Iar;  return TRUE;
    case cpu_addr_stack: addr->Offset = ctx->Gpr1; return TRUE;
    /* ELFv2 has no dedicated frame pointer: r1 addresses the frame too */
    case cpu_addr_frame: addr->Offset = ctx->Gpr1; return TRUE;
#endif
    default: addr->Mode = -1;
        return FALSE;
    }
}

#ifdef __powerpc64__
enum st_mode {stm_start, stm_ppc64, stm_done};

/* indexes in Reserved array */
#define __CurrentModeCount      0

#define curr_mode   (frame->Reserved[__CurrentModeCount] & 0x0F)
#define curr_count  (frame->Reserved[__CurrentModeCount] >> 4)

#define set_curr_mode(m) {frame->Reserved[__CurrentModeCount] &= ~0x0F; frame->Reserved[__CurrentModeCount] |= (m & 0x0F);}
#define inc_curr_count() (frame->Reserved[__CurrentModeCount] += 0x10)

/* fetch_next_frame()
 *
 * modify (at least) context.Iar using unwind information, either out of debug
 * info (dwarf), or by following the ELFv2 stack back chain.
 */
static BOOL fetch_next_frame(struct cpu_stack_walk* csw, union ctx *pcontext,
    DWORD_PTR curr_pc)
{
    DWORD64 xframe;
    CONTEXT *context = &pcontext->ctx;
    DWORD64 sp = context->Gpr1, back_chain, retaddr;

    /* The lr *register* is not the return address of an arbitrary frame: it
     * holds one only in the innermost frame, and only until that frame makes a
     * call of its own.  Both paths below therefore take the caller's pc from
     * where the ABI actually keeps it -- the return address column for dwarf,
     * the lr save slot in the caller's frame for the back chain -- and neither
     * uses the incoming context->Lr.  (Using it walked one frame and stopped,
     * or reported every frame with the pc of the frame below; measured with
     * probes/stack-walk.c.)
     *
     * Both paths also insist that the stack pointer strictly increases, which
     * is what bounds the walk: the stack is finite, so a walk that only ever
     * moves up it terminates.  Without that, a frame that fails to advance is
     * an infinite loop inside StackWalk64() -- the state this code was in, and
     * StackWalk64() has no frame limit of its own to stop it.  The price is
     * that a back chain crossing to a stack at a *lower* address (a Win32
     * thread's stack, walked from the unix side) truncates there instead of
     * following it; a truncated backtrace beats a debugger that never returns.
     */
    if (dwarf2_virtual_unwind(csw, curr_pc, pcontext, &xframe))
    {
        /* the return address column is 65, i.e. lr, so apply_frame_state() has
         * left the caller's pc in context->Lr */
        if (!context->Lr || xframe <= sp)
        {
            TRACE("dwarf unwind stops here: pc=%I64x cfa=%I64x sp=%I64x\n",
                  context->Lr, xframe, sp);
            return FALSE;
        }
        context->Iar  = context->Lr;
        context->Gpr1 = xframe;
        return TRUE;
    }

    /* No unwind info: walk the back chain instead.  In the ELFv2 ABI the word
     * at 0(r1) is the caller's stack pointer, and the return address into that
     * caller lives at 16(that pointer): the lr save slot belongs to the
     * caller's frame and is written by the prologue of whichever function it
     * calls.  A leaf never writes it, so for a leaf innermost frame the value
     * is stale -- but even then it is a return address *into the same caller*,
     * because that slot only ever receives return addresses into the frame
     * that owns it, so the frame identified is still the right one.
     */
    if (!sw_read_mem(csw, sp, &back_chain, sizeof(back_chain)) || back_chain <= sp)
        return FALSE;
    if (!sw_read_mem(csw, back_chain + 16, &retaddr, sizeof(retaddr)) || !retaddr)
        return FALSE;

    context->Iar  = retaddr;
    /* the frame we just moved into reached the one below through a bl, so at
     * this pc its lr is that same return address */
    context->Lr   = retaddr;
    context->Gpr1 = back_chain;
    return TRUE;
}

static BOOL ppc64_stack_walk(struct cpu_stack_walk *csw, STACKFRAME64 *frame,
    union ctx *context)
{
    unsigned deltapc = curr_count <= 1 ? 0 : 4;

    /* sanity check */
    if (curr_mode >= stm_done) return FALSE;

    TRACE("Enter: PC=%s Frame=%s Return=%s Stack=%s Mode=%s Count=%I64u\n",
          wine_dbgstr_addr(&frame->AddrPC),
          wine_dbgstr_addr(&frame->AddrFrame),
          wine_dbgstr_addr(&frame->AddrReturn),
          wine_dbgstr_addr(&frame->AddrStack),
          curr_mode == stm_start ? "start" : "PPC64",
          curr_count);

    if (curr_mode == stm_start)
    {
        /* Init done */
        set_curr_mode(stm_ppc64);
        frame->AddrReturn.Mode = frame->AddrStack.Mode = AddrModeFlat;
        /* don't set up AddrStack on first call. Either the caller has set it up, or
         * we will get it in the next frame
         */
        memset(&frame->AddrBStore, 0, sizeof(frame->AddrBStore));
    }
    else
    {
        if (context->ctx.Gpr1 != frame->AddrStack.Offset) FIXME("inconsistent Stack Pointer\n");
        if (context->ctx.Iar != frame->AddrPC.Offset) FIXME("inconsistent Program Counter\n");

        if (frame->AddrReturn.Offset == 0) goto done_err;
        if (!fetch_next_frame(csw, context, frame->AddrPC.Offset - deltapc))
            goto done_err;
    }

    memset(&frame->Params, 0, sizeof(frame->Params));

    /* set frame information */
    frame->AddrStack.Offset = context->ctx.Gpr1;
    frame->AddrFrame.Offset = context->ctx.Gpr1;
    frame->AddrPC.Offset = context->ctx.Iar;

    /* AddrReturn has to come from one frame further up, as on x86_64: this
     * frame's lr is not its return address unless it is a leaf, and callers
     * (including the next iteration, which stops on a zero return address)
     * rely on it naming where this frame really returns to. */
    {
        union ctx newctx = *context;

        frame->AddrReturn.Mode = AddrModeFlat;
        if (fetch_next_frame(csw, &newctx, frame->AddrPC.Offset - deltapc))
            frame->AddrReturn.Offset = newctx.ctx.Iar;
        else
            frame->AddrReturn.Offset = 0;
    }

    frame->Far = TRUE;
    frame->Virtual = TRUE;
    inc_curr_count();

    TRACE("Leave: PC=%s Frame=%s Return=%s Stack=%s Mode=%s Count=%I64u FuncTable=%p\n",
          wine_dbgstr_addr(&frame->AddrPC),
          wine_dbgstr_addr(&frame->AddrFrame),
          wine_dbgstr_addr(&frame->AddrReturn),
          wine_dbgstr_addr(&frame->AddrStack),
          curr_mode == stm_start ? "start" : "PPC64",
          curr_count,
          frame->FuncTableEntry);

    return TRUE;
done_err:
    set_curr_mode(stm_done);
    return FALSE;
}
#else
static BOOL ppc64_stack_walk(struct cpu_stack_walk* csw, STACKFRAME64 *frame,
    union ctx *ctx)
{
    return FALSE;
}
#endif

static unsigned ppc64_map_dwarf_register(unsigned regno, const struct module* module, BOOL eh_frame)
{
    if (regno <= 31) return CV_PPC_GPR0 + regno;
    if (regno >= 32 && regno <= 63) return CV_PPC_FPR0 + regno - 32;
    if (regno == PPC64_DWARF_LR) return CV_PPC_LR;
    if (regno == PPC64_DWARF_CTR) return CV_PPC_CTR;
    if (regno >= PPC64_DWARF_CR0 && regno <= PPC64_DWARF_CR0 + 7)
        return CV_PPC_CR0 + regno - PPC64_DWARF_CR0;
    if (regno == PPC64_DWARF_XER) return CV_PPC_XER;
    /* v0-v31 (77-108), vrsave (109) and vscr (110) have no CV_PPC number at all
     * -- the codeview PowerPC enumeration predates VMX -- so they cannot be
     * named or fetched here even though CONTEXT does carry Vr[]/Vrsave/Vscr.
     * gcc 16 emits DW_CFA_offset for v25-v31 (columns 102-108) in ordinary -O2
     * code, so this is a routine occurrence, not a corrupt-file case: the rule
     * is dropped and the unwind proceeds without it, which costs nothing that
     * a stack walk uses.  Keep it quiet at TRACE level for that reason. */
    if (regno >= 77 && regno <= 110)
    {
        TRACE("No CV register for dwarf register %d (vector state)\n", regno);
        return CV_REG_NONE;
    }

    FIXME("Don't know how to map register %d\n", regno);
    return CV_REG_NONE;
}

static void *ppc64_fetch_context_reg(union ctx *pctx, unsigned regno, unsigned *size)
{
#ifdef __powerpc64__
    CONTEXT *ctx = &pctx->ctx;

    /* Gpr0..Gpr31 and Fpr0..Fpr31 are declared as consecutive same-typed
     * members, so they can be indexed as arrays. Make sure of it. */
    C_ASSERT(offsetof(CONTEXT, Gpr31) - offsetof(CONTEXT, Gpr0) == 31 * sizeof(DWORD64));
    C_ASSERT(offsetof(CONTEXT, Fpr31) - offsetof(CONTEXT, Fpr0) == 31 * sizeof(double));
#endif

    /* Set first, and unconditionally: every failure path below returns NULL,
     * and dwarf.c's set_context_reg() used to memcpy() through that NULL with
     * whatever *size happened to contain.  A zero size plus a NULL pointer is
     * the "no storage for this register" answer its callers now expect. */
    *size = 0;

#ifdef __powerpc64__
    if (regno >= CV_PPC_GPR0 && regno <= CV_PPC_GPR0 + 31)
    {
        *size = sizeof(DWORD64);
        return &(&ctx->Gpr0)[regno - CV_PPC_GPR0];
    }
    if (regno >= CV_PPC_FPR0 && regno <= CV_PPC_FPR0 + 31)
    {
        *size = sizeof(double);
        return &(&ctx->Fpr0)[regno - CV_PPC_FPR0];
    }
    switch (regno)
    {
    case CV_PPC_CR:    *size = sizeof(ctx->Cr);    return &ctx->Cr;
    case CV_PPC_XER:   *size = sizeof(ctx->Xer);   return &ctx->Xer;
    case CV_PPC_MSR:   *size = sizeof(ctx->Msr);   return &ctx->Msr;
    case CV_PPC_PC:    *size = sizeof(ctx->Iar);   return &ctx->Iar;
    case CV_PPC_LR:    *size = sizeof(ctx->Lr);    return &ctx->Lr;
    case CV_PPC_CTR:   *size = sizeof(ctx->Ctr);   return &ctx->Ctr;
    case CV_PPC_FPSCR: *size = sizeof(ctx->Fpscr); return &ctx->Fpscr;
    }
    /* cr0-cr7 are mapped (so that traces can name them) but are 4-bit fields of
     * ctx->Cr, not objects: there is nothing to hand back a pointer to, and
     * writing the saved word into Cr would clobber the seven other fields. */
    if (regno >= CV_PPC_CR0 && regno <= CV_PPC_CR0 + 7)
    {
        TRACE("No separate storage for cr%u in CONTEXT\n", regno - CV_PPC_CR0);
        return NULL;
    }
    if (regno == CV_REG_NONE) return NULL;  /* already diagnosed by the mapper */
#endif
    FIXME("Unknown register %x\n", regno);
    return NULL;
}

static const char* ppc64_fetch_regname(unsigned regno)
{
    static const char * const gpr_names[32] =
    {
        "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
        "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
        "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
        "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
    };
    static const char * const fpr_names[32] =
    {
        "f0",  "f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7",
        "f8",  "f9",  "f10", "f11", "f12", "f13", "f14", "f15",
        "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
        "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31",
    };
    static const char * const cr_names[8] =
    {
        "cr0", "cr1", "cr2", "cr3", "cr4", "cr5", "cr6", "cr7",
    };

    if (regno >= CV_PPC_GPR0 && regno <= CV_PPC_GPR0 + 31) return gpr_names[regno - CV_PPC_GPR0];
    if (regno >= CV_PPC_FPR0 && regno <= CV_PPC_FPR0 + 31) return fpr_names[regno - CV_PPC_FPR0];
    if (regno >= CV_PPC_CR0 && regno <= CV_PPC_CR0 + 7) return cr_names[regno - CV_PPC_CR0];
    switch (regno)
    {
    case CV_PPC_CR:    return "cr";
    case CV_PPC_XER:   return "xer";
    case CV_PPC_MSR:   return "msr";
    case CV_PPC_PC:    return "pc";
    case CV_PPC_LR:    return "lr";
    case CV_PPC_CTR:   return "ctr";
    case CV_PPC_FPSCR: return "fpscr";
    }
    FIXME("Unknown register %x\n", regno);
    return NULL;
}

static BOOL ppc64_fetch_minidump_thread(struct dump_context* dc, unsigned index, unsigned flags, const CONTEXT* ctx)
{
    if (ctx->ContextFlags && (flags & ThreadWriteInstructionWindow))
    {
        /* FIXME: crop values across module boundaries, */
#ifdef __powerpc64__
        ULONG64 base = ctx->Iar <= 0x80 ? 0 : ctx->Iar - 0x80;
        minidump_add_memory_block(dc, base, ctx->Iar + 0x80 - base, 0);
#endif
    }

    return TRUE;
}

static BOOL ppc64_fetch_minidump_module(struct dump_context* dc, unsigned index, unsigned flags)
{
    /* FIXME: actually, we should probably take care of FPO data, unless it's stored in
     * function table minidump stream
     */
    return FALSE;
}

struct cpu cpu_ppc64 = {
    IMAGE_FILE_MACHINE_POWERPC64,
    8,
    CV_PPC_GPR0 + 1, /* r1 */
    ppc64_get_addr,
    ppc64_stack_walk,
    NULL,
    ppc64_map_dwarf_register,
    ppc64_fetch_context_reg,
    ppc64_fetch_regname,
    ppc64_fetch_minidump_thread,
    ppc64_fetch_minidump_module,
};
