/*
 * File cpu_riscv.c
 *
 * RISC-V 64-bit CPU support for dbghelp.
 *
 * This initial backend supports native RISC-V register access and
 * DWARF-based stack walking. Its private register identifiers are
 * not Windows CodeView register identifiers.
 */

#include <string.h>

#include "ntstatus.h"
#include "dbghelp_private.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

/*
 * Private register identifiers used by this backend.
 * DWARF RISC-V register numbers:
 *   0-31  = x0-x31
 *   32-63 = f0-f31
 *   64    = PC
 *   65    = FCSR
 */
#define RISCV_GPR_BASE  0x1000
#define RISCV_FPR_BASE  0x1040
#define RISCV_PC_REG    0x1080
#define RISCV_FCSR_REG  0x1081

#define RISCV_GPR(n) (RISCV_GPR_BASE + (n))
#define RISCV_FPR(n) (RISCV_FPR_BASE + (n))

static BOOL riscv_get_addr(HANDLE hThread, const CONTEXT *ctx,
                           enum cpu_addr ca, ADDRESS64 *addr)
{
    (void)hThread;

    addr->Mode = AddrModeFlat;
    addr->Segment = 0;

#if defined(__riscv) && __riscv_xlen == 64
    switch (ca)
    {
    case cpu_addr_pc:
        addr->Offset = ctx->Pc;
        return TRUE;
    case cpu_addr_stack:
        addr->Offset = ctx->Gpr.X[2]; /* sp */
        return TRUE;
    case cpu_addr_frame:
        addr->Offset = ctx->Gpr.X[8]; /* s0/fp */
        return TRUE;
    }
#endif

    addr->Mode = -1;
    return FALSE;
}

#ifdef __riscv

#if __riscv_xlen == 64

enum riscv_stack_mode
{
    RISCV_ST_START,
    RISCV_ST_WALK,
    RISCV_ST_DONE
};

#define riscv_mode(frame)  ((frame)->Reserved[0] & 0x0f)
#define riscv_count(frame) ((frame)->Reserved[0] >> 4)

static void riscv_set_mode(STACKFRAME64 *frame, unsigned mode)
{
    frame->Reserved[0] = (frame->Reserved[0] & ~(DWORD64)0x0f) | (mode & 0x0f);
}

static void riscv_inc_count(STACKFRAME64 *frame)
{
    frame->Reserved[0] += 0x10;
}

static BOOL riscv_fetch_next_frame(struct cpu_stack_walk *csw,
                                   union ctx *pcontext, DWORD_PTR curr_pc)
{
    CONTEXT *ctx = &pcontext->ctx;
    DWORD64 cfa;
    DWORD64 old_return = ctx->Gpr.X[1]; /* ra */

    /*
     * RISC-V instructions can be 2 or 4 bytes long. Use the current
     * PC for the first frame; for subsequent return addresses, use
     * an address just inside the preceding instruction's range.
     */
    if (dwarf2_virtual_unwind(csw, curr_pc, pcontext, &cfa))
    {
        ctx->Gpr.X[2] = cfa;
        ctx->Pc = old_return;
        return TRUE;
    }

    /* Without unwind information, allow a limited return-address fallback. */
    if (!old_return || ctx->Pc == old_return)
        return FALSE;

    ctx->Pc = old_return;
    return TRUE;
}

static BOOL riscv_stack_walk(struct cpu_stack_walk *csw,
                             STACKFRAME64 *frame, union ctx *pcontext)
{
    CONTEXT *ctx = &pcontext->ctx;
    DWORD64 curr_pc;

    if (riscv_mode(frame) >= RISCV_ST_DONE)
        return FALSE;

    if (riscv_mode(frame) == RISCV_ST_START)
    {
        riscv_set_mode(frame, RISCV_ST_WALK);
        frame->AddrReturn.Mode = AddrModeFlat;
        frame->AddrStack.Mode = AddrModeFlat;
        memset(&frame->AddrBStore, 0, sizeof(frame->AddrBStore));
    }
    else
    {
        if (!frame->AddrReturn.Offset)
            goto done;

        curr_pc = frame->AddrPC.Offset;

        /* RISC-V has 16-bit compressed instructions, so adjust by one byte. */
        if (riscv_count(frame) > 1 && curr_pc)
            curr_pc--;

        if (!riscv_fetch_next_frame(csw, pcontext, curr_pc))
            goto done;
    }

    memset(&frame->Params, 0, sizeof(frame->Params));

    frame->AddrPC.Offset = ctx->Pc;
    frame->AddrStack.Offset = ctx->Gpr.X[2];
    frame->AddrFrame.Offset = ctx->Gpr.X[8];
    frame->AddrReturn.Offset = ctx->Gpr.X[1];

    frame->Far = TRUE;
    frame->Virtual = TRUE;

    riscv_inc_count(frame);
    return TRUE;

done:
    riscv_set_mode(frame, RISCV_ST_DONE);
    return FALSE;
}

#else
static BOOL riscv_stack_walk(struct cpu_stack_walk *csw,
                             STACKFRAME64 *frame, union ctx *context)
{
    (void)csw;
    (void)frame;
    (void)context;
    return FALSE;
}
#endif
#else
static BOOL riscv_stack_walk(struct cpu_stack_walk *csw,
                             STACKFRAME64 *frame, union ctx *context)
{
    (void)csw;
    (void)frame;
    (void)context;
    return FALSE;
}
#endif

static unsigned riscv_map_dwarf_register(unsigned regno,
                                         const struct module *module,
                                         BOOL eh_frame)
{
    (void)module;
    (void)eh_frame;

    if (regno <= 31)
        return RISCV_GPR(regno);
    if (regno >= 32 && regno <= 63)
        return RISCV_FPR(regno - 32);
    if (regno == 64)
        return RISCV_PC_REG;
    if (regno == 65)
        return RISCV_FCSR_REG;

    FIXME("Unsupported RISC-V DWARF register %u\n", regno);
    return 0;
}

static void *riscv_fetch_context_reg(union ctx *pcontext,
                                     unsigned regno, unsigned *size)
{
#if defined(__riscv) && __riscv_xlen == 64
    CONTEXT *ctx = &pcontext->ctx;

    if (regno >= RISCV_GPR_BASE && regno < RISCV_GPR_BASE + 32)
    {
        *size = sizeof(ctx->Gpr.X[0]);
        return &ctx->Gpr.X[regno - RISCV_GPR_BASE];
    }

    if (regno >= RISCV_FPR_BASE && regno < RISCV_FPR_BASE + 32)
    {
        *size = sizeof(ctx->F[0]);
        return &ctx->F[regno - RISCV_FPR_BASE];
    }

    switch (regno)
    {
    case RISCV_PC_REG:
        *size = sizeof(ctx->Pc);
        return &ctx->Pc;
    case RISCV_FCSR_REG:
        *size = sizeof(ctx->Fcsr);
        return &ctx->Fcsr;
    }
#else
    (void)pcontext;
#endif

    FIXME("Unknown RISC-V context register %#x\n", regno);
    return NULL;
}

static const char *riscv_fetch_regname(unsigned regno)
{
    static const char * const gpr_names[32] =
    {
        "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
        "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
        "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
        "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
    };
    static const char * const fpr_names[32] =
    {
        "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
        "f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",
        "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
        "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31"
    };

    if (regno >= RISCV_GPR_BASE && regno < RISCV_GPR_BASE + 32)
        return gpr_names[regno - RISCV_GPR_BASE];

    if (regno >= RISCV_FPR_BASE && regno < RISCV_FPR_BASE + 32)
        return fpr_names[regno - RISCV_FPR_BASE];

    if (regno == RISCV_PC_REG)
        return "pc";
    if (regno == RISCV_FCSR_REG)
        return "fcsr";

    FIXME("Unknown RISC-V register name %#x\n", regno);
    return NULL;
}

static BOOL riscv_fetch_minidump_thread(struct dump_context *dc,
                                        unsigned index, unsigned flags,
                                        const CONTEXT *ctx)
{
    (void)index;

    if (ctx->ContextFlags && (flags & ThreadWriteInstructionWindow))
    {
#if defined(__riscv) && __riscv_xlen == 64
        ULONG64 base = ctx->Pc <= 0x80 ? 0 : ctx->Pc - 0x80;
        minidump_add_memory_block(dc, base, ctx->Pc + 0x80 - base, 0);
#endif
    }

    return TRUE;
}

static BOOL riscv_fetch_minidump_module(struct dump_context *dc,
                                        unsigned index, unsigned flags)
{
    (void)dc;
    (void)index;
    (void)flags;
    return FALSE;
}

struct cpu cpu_riscv =
{
    IMAGE_FILE_MACHINE_RISCV64,
    8,
    RISCV_GPR(8),
    riscv_get_addr,
    riscv_stack_walk,
    NULL,
    riscv_map_dwarf_register,
    riscv_fetch_context_reg,
    riscv_fetch_regname,
    riscv_fetch_minidump_thread,
    riscv_fetch_minidump_module
};
