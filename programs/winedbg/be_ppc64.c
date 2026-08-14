/*
 * Debugger PowerPC64 specific functions
 *
 * Copyright 2010-2013 André Hentschel
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

#include "debugger.h"

#ifdef __powerpc64__

/* the "trap" instruction (tw 31,r0,r0), which is what ntdll's DbgBreakPoint()
 * and the SIGTRAP handler in signal_ppc64.c agree on as the breakpoint insn */
#define PPC64_TRAP_INSN 0x7fe00008

static BOOL be_ppc64_get_addr(HANDLE hThread, const dbg_ctx_t *ctx,
                              enum be_cpu_addr bca, ADDRESS64* addr)
{
    switch (bca)
    {
    case be_cpu_addr_pc:
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->ctx.Iar);
    case be_cpu_addr_stack:
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->ctx.Gpr1);
    case be_cpu_addr_frame:
        /* ELFv2 has no dedicated frame pointer; r1 addresses the frame */
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->ctx.Gpr1);
    }
    return FALSE;
}

static BOOL be_ppc64_get_register_info(int regno, enum be_cpu_addr* kind)
{
    switch (regno)
    {
    case CV_PPC_PC:         *kind = be_cpu_addr_pc;    return TRUE;
    case CV_PPC_GPR0 + 1:   *kind = be_cpu_addr_stack; return TRUE;
    }
    return FALSE;
}

static void be_ppc64_single_step(dbg_ctx_t *ctx, BOOL enable)
{
    /* MSR[SE] (bit 21 counting from the MSB, i.e. 0x400) is the single step
     * enable, but it is privileged: a user-mode SetThreadContext() cannot
     * change it. Single stepping needs PTRACE_SINGLESTEP, which winedbg's
     * Win32 debugging interface does not expose here. */
    dbg_printf("be_ppc64_single_step: not done\n");
}

static void be_ppc64_print_context(HANDLE hThread, const dbg_ctx_t *ctx, int all_regs)
{
    static const char condflags[] = "LGEO";
    int i;
    char buf[8];

    dbg_printf("Register dump:\n");

    /* condition register field 0, as set by the record forms */
    strcpy(buf, condflags);
    for (i = 0; buf[i]; i++)
        if (!(ctx->ctx.Cr & (0x80000000 >> i)))
            buf[i] = '-';

    dbg_printf(" Iar:%016I64x Lr:%016I64x Ctr:%016I64x Cr:%08x(%s)\n",
               ctx->ctx.Iar, ctx->ctx.Lr, ctx->ctx.Ctr, (unsigned)ctx->ctx.Cr, buf);
    dbg_printf(" Msr:%016I64x Xer:%016I64x\n", ctx->ctx.Msr, ctx->ctx.Xer);
    dbg_printf(" r0: %016I64x r1: %016I64x r2: %016I64x r3: %016I64x\n",
               ctx->ctx.Gpr0, ctx->ctx.Gpr1, ctx->ctx.Gpr2, ctx->ctx.Gpr3);
    dbg_printf(" r4: %016I64x r5: %016I64x r6: %016I64x r7: %016I64x\n",
               ctx->ctx.Gpr4, ctx->ctx.Gpr5, ctx->ctx.Gpr6, ctx->ctx.Gpr7);
    dbg_printf(" r8: %016I64x r9: %016I64x r10:%016I64x r11:%016I64x\n",
               ctx->ctx.Gpr8, ctx->ctx.Gpr9, ctx->ctx.Gpr10, ctx->ctx.Gpr11);
    dbg_printf(" r12:%016I64x r13:%016I64x r14:%016I64x r15:%016I64x\n",
               ctx->ctx.Gpr12, ctx->ctx.Gpr13, ctx->ctx.Gpr14, ctx->ctx.Gpr15);
    dbg_printf(" r16:%016I64x r17:%016I64x r18:%016I64x r19:%016I64x\n",
               ctx->ctx.Gpr16, ctx->ctx.Gpr17, ctx->ctx.Gpr18, ctx->ctx.Gpr19);
    dbg_printf(" r20:%016I64x r21:%016I64x r22:%016I64x r23:%016I64x\n",
               ctx->ctx.Gpr20, ctx->ctx.Gpr21, ctx->ctx.Gpr22, ctx->ctx.Gpr23);
    dbg_printf(" r24:%016I64x r25:%016I64x r26:%016I64x r27:%016I64x\n",
               ctx->ctx.Gpr24, ctx->ctx.Gpr25, ctx->ctx.Gpr26, ctx->ctx.Gpr27);
    dbg_printf(" r28:%016I64x r29:%016I64x r30:%016I64x r31:%016I64x\n",
               ctx->ctx.Gpr28, ctx->ctx.Gpr29, ctx->ctx.Gpr30, ctx->ctx.Gpr31);

    if (all_regs) dbg_printf( "Floating point PPC64 dump not implemented\n" );
}

static void be_ppc64_print_segment_info(HANDLE hThread, const dbg_ctx_t *ctx)
{
}

static struct dbg_internal_var be_ppc64_ctx[] =
{
    {CV_PPC_GPR0 +  0, "r0",  (void*)FIELD_OFFSET(CONTEXT, Gpr0),  dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 +  1, "r1",  (void*)FIELD_OFFSET(CONTEXT, Gpr1),  dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 +  2, "r2",  (void*)FIELD_OFFSET(CONTEXT, Gpr2),  dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 +  3, "r3",  (void*)FIELD_OFFSET(CONTEXT, Gpr3),  dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 +  4, "r4",  (void*)FIELD_OFFSET(CONTEXT, Gpr4),  dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 +  5, "r5",  (void*)FIELD_OFFSET(CONTEXT, Gpr5),  dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 +  6, "r6",  (void*)FIELD_OFFSET(CONTEXT, Gpr6),  dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 +  7, "r7",  (void*)FIELD_OFFSET(CONTEXT, Gpr7),  dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 +  8, "r8",  (void*)FIELD_OFFSET(CONTEXT, Gpr8),  dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 +  9, "r9",  (void*)FIELD_OFFSET(CONTEXT, Gpr9),  dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 + 10, "r10", (void*)FIELD_OFFSET(CONTEXT, Gpr10), dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 + 11, "r11", (void*)FIELD_OFFSET(CONTEXT, Gpr11), dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 + 12, "r12", (void*)FIELD_OFFSET(CONTEXT, Gpr12), dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 + 13, "r13", (void*)FIELD_OFFSET(CONTEXT, Gpr13), dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 + 14, "r14", (void*)FIELD_OFFSET(CONTEXT, Gpr14), dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 + 15, "r15", (void*)FIELD_OFFSET(CONTEXT, Gpr15), dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 + 16, "r16", (void*)FIELD_OFFSET(CONTEXT, Gpr16), dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 + 17, "r17", (void*)FIELD_OFFSET(CONTEXT, Gpr17), dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 + 18, "r18", (void*)FIELD_OFFSET(CONTEXT, Gpr18), dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 + 19, "r19", (void*)FIELD_OFFSET(CONTEXT, Gpr19), dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 + 20, "r20", (void*)FIELD_OFFSET(CONTEXT, Gpr20), dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 + 21, "r21", (void*)FIELD_OFFSET(CONTEXT, Gpr21), dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 + 22, "r22", (void*)FIELD_OFFSET(CONTEXT, Gpr22), dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 + 23, "r23", (void*)FIELD_OFFSET(CONTEXT, Gpr23), dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 + 24, "r24", (void*)FIELD_OFFSET(CONTEXT, Gpr24), dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 + 25, "r25", (void*)FIELD_OFFSET(CONTEXT, Gpr25), dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 + 26, "r26", (void*)FIELD_OFFSET(CONTEXT, Gpr26), dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 + 27, "r27", (void*)FIELD_OFFSET(CONTEXT, Gpr27), dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 + 28, "r28", (void*)FIELD_OFFSET(CONTEXT, Gpr28), dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 + 29, "r29", (void*)FIELD_OFFSET(CONTEXT, Gpr29), dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 + 30, "r30", (void*)FIELD_OFFSET(CONTEXT, Gpr30), dbg_itype_unsigned_int64},
    {CV_PPC_GPR0 + 31, "r31", (void*)FIELD_OFFSET(CONTEXT, Gpr31), dbg_itype_unsigned_int64},
    {CV_PPC_CR,        "cr",  (void*)FIELD_OFFSET(CONTEXT, Cr),    dbg_itype_unsigned_int64},
    {CV_PPC_XER,       "xer", (void*)FIELD_OFFSET(CONTEXT, Xer),   dbg_itype_unsigned_int64},
    {CV_PPC_MSR,       "msr", (void*)FIELD_OFFSET(CONTEXT, Msr),   dbg_itype_unsigned_int64},
    {CV_PPC_PC,        "pc",  (void*)FIELD_OFFSET(CONTEXT, Iar),   dbg_itype_unsigned_int64},
    {CV_PPC_LR,        "lr",  (void*)FIELD_OFFSET(CONTEXT, Lr),    dbg_itype_unsigned_int64},
    {CV_PPC_CTR,       "ctr", (void*)FIELD_OFFSET(CONTEXT, Ctr),   dbg_itype_unsigned_int64},
    {0,                NULL,  0,                                   dbg_itype_none}
};

static BOOL be_ppc64_is_step_over_insn(const void* insn)
{
    dbg_printf("be_ppc64_is_step_over_insn: not done\n");
    return FALSE;
}

static BOOL be_ppc64_is_function_return(const void* insn)
{
    dbg_printf("be_ppc64_is_function_return: not done\n");
    return FALSE;
}

static BOOL be_ppc64_is_break_insn(const void* insn)
{
    return *(const unsigned int *)insn == PPC64_TRAP_INSN;
}

static BOOL be_ppc64_is_func_call(const void* insn, ADDRESS64* callee)
{
    return FALSE;
}

static BOOL be_ppc64_is_jump(const void* insn, ADDRESS64* jumpee)
{
    return FALSE;
}

static BOOL be_ppc64_insert_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                   dbg_ctx_t *ctx, enum be_xpoint_type type,
                                   void* addr, unsigned *val, unsigned size)
{
    unsigned int    trap = PPC64_TRAP_INSN;
    unsigned int    old;
    SIZE_T          sz;

    switch (type)
    {
    case be_xpoint_break:
        if (!size) return FALSE;
        if (!pio->read(hProcess, addr, &old, 4, &sz) || sz != 4) return FALSE;
        if (!pio->write(hProcess, addr, &trap, 4, &sz) || sz != 4) return FALSE;
        *val = old;
        break;
    default:
        dbg_printf("Unknown/unsupported bp type %c\n", type);
        return FALSE;
    }
    return TRUE;
}

static BOOL be_ppc64_remove_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                   dbg_ctx_t *ctx, enum be_xpoint_type type,
                                   void* addr, unsigned val, unsigned size)
{
    SIZE_T          sz;

    switch (type)
    {
    case be_xpoint_break:
        if (!size) return FALSE;
        if (!pio->write(hProcess, addr, &val, 4, &sz) || sz != 4) return FALSE;
        break;
    default:
        dbg_printf("Unknown/unsupported bp type %c\n", type);
        return FALSE;
    }
    return TRUE;
}

static BOOL be_ppc64_is_watchpoint_set(const dbg_ctx_t *ctx, unsigned idx)
{
    dbg_printf("be_ppc64_is_watchpoint_set: not done\n");
    return FALSE;
}

static void be_ppc64_clear_watchpoint(dbg_ctx_t *ctx, unsigned idx)
{
    dbg_printf("be_ppc64_clear_watchpoint: not done\n");
}

static int be_ppc64_adjust_pc_for_break(dbg_ctx_t *ctx, BOOL way)
{
    /* the SIGTRAP handler advances Iar past the trap instruction, so going
     * "back onto" the breakpoint means stepping back one instruction */
    if (way)
    {
        ctx->ctx.Iar -= 4;
        return -4;
    }
    ctx->ctx.Iar += 4;
    return 4;
}

void be_ppc64_disasm_one_insn(ADDRESS64 *addr, int display)
{
    /* The bundled capstone build has no PowerPC backend (libs/capstone only
     * carries AArch64/ARM/X86), so there is nothing to disassemble with. Print
     * the raw instruction word rather than pretend. */
    if (display)
    {
        unsigned int    insn;
        SIZE_T          len;

        if (dbg_curr_process->process_io->read( dbg_curr_process->handle,
                                                memory_to_linear_addr(addr),
                                                &insn, sizeof(insn), &len ) && len == sizeof(insn))
            dbg_printf( ".long 0x%08x", insn );
        else
            dbg_printf( "<cannot read instruction>" );
    }
    addr->Offset += 4;
}

static BOOL be_ppc64_get_context(HANDLE thread, dbg_ctx_t *ctx)
{
    ctx->ctx.ContextFlags = CONTEXT_ALL;
    return GetThreadContext(thread, &ctx->ctx);
}

static BOOL be_ppc64_set_context(HANDLE thread, const dbg_ctx_t *ctx)
{
    return SetThreadContext(thread, &ctx->ctx);
}

#define REG(f,n,t,r)  {f, n, t, FIELD_OFFSET(CONTEXT, r), sizeof(((CONTEXT*)NULL)->r)}

static struct gdb_register be_ppc64_gdb_register_map[] = {
    REG("core", "r0",   NULL,         Gpr0),
    REG(NULL,   "r1",   "data_ptr",   Gpr1),
    REG(NULL,   "r2",   NULL,         Gpr2),
    REG(NULL,   "r3",   NULL,         Gpr3),
    REG(NULL,   "r4",   NULL,         Gpr4),
    REG(NULL,   "r5",   NULL,         Gpr5),
    REG(NULL,   "r6",   NULL,         Gpr6),
    REG(NULL,   "r7",   NULL,         Gpr7),
    REG(NULL,   "r8",   NULL,         Gpr8),
    REG(NULL,   "r9",   NULL,         Gpr9),
    REG(NULL,   "r10",  NULL,         Gpr10),
    REG(NULL,   "r11",  NULL,         Gpr11),
    REG(NULL,   "r12",  NULL,         Gpr12),
    REG(NULL,   "r13",  NULL,         Gpr13),
    REG(NULL,   "r14",  NULL,         Gpr14),
    REG(NULL,   "r15",  NULL,         Gpr15),
    REG(NULL,   "r16",  NULL,         Gpr16),
    REG(NULL,   "r17",  NULL,         Gpr17),
    REG(NULL,   "r18",  NULL,         Gpr18),
    REG(NULL,   "r19",  NULL,         Gpr19),
    REG(NULL,   "r20",  NULL,         Gpr20),
    REG(NULL,   "r21",  NULL,         Gpr21),
    REG(NULL,   "r22",  NULL,         Gpr22),
    REG(NULL,   "r23",  NULL,         Gpr23),
    REG(NULL,   "r24",  NULL,         Gpr24),
    REG(NULL,   "r25",  NULL,         Gpr25),
    REG(NULL,   "r26",  NULL,         Gpr26),
    REG(NULL,   "r27",  NULL,         Gpr27),
    REG(NULL,   "r28",  NULL,         Gpr28),
    REG(NULL,   "r29",  NULL,         Gpr29),
    REG(NULL,   "r30",  NULL,         Gpr30),
    REG(NULL,   "r31",  NULL,         Gpr31),
    REG(NULL,   "pc",   "code_ptr",   Iar),
    REG(NULL,   "msr",  NULL,         Msr),
    REG(NULL,   "cr",   NULL,         Cr),
    REG(NULL,   "lr",   "code_ptr",   Lr),
    REG(NULL,   "ctr",  NULL,         Ctr),
    REG(NULL,   "xer",  NULL,         Xer),
};

struct backend_cpu be_ppc64 =
{
    IMAGE_FILE_MACHINE_POWERPC64,
    8,
    be_cpu_linearize,
    be_cpu_build_addr,
    be_ppc64_get_addr,
    be_ppc64_get_register_info,
    be_ppc64_single_step,
    be_ppc64_print_context,
    be_ppc64_print_segment_info,
    be_ppc64_ctx,
    be_ppc64_is_step_over_insn,
    be_ppc64_is_function_return,
    be_ppc64_is_break_insn,
    be_ppc64_is_func_call,
    be_ppc64_is_jump,
    be_ppc64_disasm_one_insn,
    be_ppc64_insert_Xpoint,
    be_ppc64_remove_Xpoint,
    be_ppc64_is_watchpoint_set,
    be_ppc64_clear_watchpoint,
    be_ppc64_adjust_pc_for_break,
    be_ppc64_get_context,
    be_ppc64_set_context,
    be_ppc64_gdb_register_map,
    ARRAY_SIZE(be_ppc64_gdb_register_map),
};
#endif
