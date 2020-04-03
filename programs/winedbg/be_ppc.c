/*
 * Debugger Power PC specific functions
 *
 * Copyright 2000-2003 Marcus Meissner
 *                2004 Eric Pouech
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

#if defined(__powerpc__)

static BOOL be_ppc_get_addr(HANDLE hThread, const dbg_ctx_t *ctx,
                            enum be_cpu_addr bca, ADDRESS64* addr)
{
    switch (bca)
    {
    case be_cpu_addr_pc:
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->ctx.Iar);
    default:
    case be_cpu_addr_stack:
    case be_cpu_addr_frame:
        dbg_printf("not done\n");
    }
    return FALSE;
}

static BOOL be_ppc_get_register_info(int regno, enum be_cpu_addr* kind)
{
    dbg_printf("not done\n");
    return FALSE;
}

static void be_ppc_single_step(dbg_ctx_t *ctx, BOOL enable)
{
#ifndef MSR_SE
# define MSR_SE (1<<10)
#endif 
    if (enable) ctx->ctx.Msr |= MSR_SE;
    else ctx->ctx.Msr &= ~MSR_SE;
}

static void be_ppc_print_context(HANDLE hThread, const dbg_ctx_t *ctx, int all_regs)
{
    dbg_printf("Context printing for PPC not done yet\n");
}

static void be_ppc_print_segment_info(HANDLE hThread, const dbg_ctx_t *ctx)
{
}

static struct dbg_internal_var be_ppc_ctx[] =
{
    {0,                 NULL,           0,                                      dbg_itype_none}
};

static BOOL be_ppc_is_step_over_insn(const void* insn)
{
    dbg_printf("not done\n");
    return FALSE;
}

static BOOL be_ppc_is_function_return(const void* insn)
{
    dbg_printf("not done\n");
    return FALSE;
}

static BOOL be_ppc_is_break_insn(const void* insn)
{
    dbg_printf("not done\n");
    return FALSE;
}

static BOOL be_ppc_is_func_call(const void* insn, ADDRESS64* callee)
{
    return FALSE;
}

static BOOL be_ppc_is_jump(const void* insn, ADDRESS64* jumpee)
{
    return FALSE;
}

static void be_ppc_disasm_one_insn(ADDRESS64* addr, int display)

{
    dbg_printf("Disasm NIY\n");
}

static BOOL be_ppc_insert_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                 dbg_ctx_t *ctx, enum be_xpoint_type type,
                                 void* addr, unsigned long* val, unsigned size)
{
    unsigned long       xbp;
    SIZE_T              sz;

    switch (type)
    {
    case be_xpoint_break:
        if (!size) return FALSE;
        if (!pio->read(hProcess, addr, val, 4, &sz) || sz != 4) return FALSE;
        xbp = 0x7d821008; /* 7d 82 10 08 ... in big endian */
        if (!pio->write(hProcess, addr, &xbp, 4, &sz) || sz != 4) return FALSE;
        break;
    default:
        dbg_printf("Unknown/unsupported bp type %c\n", type);
        return FALSE;
    }
    return TRUE;
}

static BOOL be_ppc_remove_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                 dbg_ctx_t *ctx, enum be_xpoint_type type,
                                 void* addr, unsigned long val, unsigned size)
{
    SIZE_T              sz;

    switch (type)
    {
    case be_xpoint_break:
        if (!size) return FALSE;
        if (!pio->write(hProcess, addr, &val, 4, &sz) || sz == 4) return FALSE;
        break;
    default:
        dbg_printf("Unknown/unsupported bp type %c\n", type);
        return FALSE;
    }
    return TRUE;
}

static BOOL be_ppc_is_watchpoint_set(const dbg_ctx_t *ctx, unsigned idx)
{
    dbg_printf("not done\n");
    return FALSE;
}

static void be_ppc_clear_watchpoint(dbg_ctx_t *ctx, unsigned idx)
{
    dbg_printf("not done\n");
}

static int be_ppc_adjust_pc_for_break(dbg_ctx_t *ctx, BOOL way)
{
    dbg_printf("not done\n");
    return 0;
}

static BOOL be_ppc_fetch_integer(const struct dbg_lvalue* lvalue, unsigned size,
                                 BOOL is_signed, LONGLONG* ret)
{
    dbg_printf("not done\n");
    return FALSE;
}

static BOOL be_ppc_fetch_float(const struct dbg_lvalue* lvalue, unsigned size,
                               long double* ret)
{
    dbg_printf("not done\n");
    return FALSE;
}

static BOOL be_ppc_store_integer(const struct dbg_lvalue* lvalue, unsigned size,
                                 BOOL is_signed, LONGLONG val)
{
    dbg_printf("be_ppc_store_integer: not done\n");
    return FALSE;
}

static BOOL be_ppc_get_context(HANDLE thread, dbg_ctx_t *ctx)
{
    ctx->ctx.ContextFlags = CONTEXT_ALL;
    return GetThreadContext(thread, &ctx->ctx);
}

static BOOL be_ppc_set_context(HANDLE thread, const dbg_ctx_t *ctx)
{
    return SetThreadContext(thread, &ctx->ctx);
}

#define REG(f,n,t,r)  {f, n, t, FIELD_OFFSET(CONTEXT, r), sizeof(((CONTEXT*)NULL)->r)}

static struct gdb_register be_ppc_gdb_register_map[] = {
    REG("core", "r0",  NULL,       Gpr0),
    REG(NULL,   "r1",  NULL,       Gpr1),
    REG(NULL,   "r2",  NULL,       Gpr2),
    REG(NULL,   "r3",  NULL,       Gpr3),
    REG(NULL,   "r4",  NULL,       Gpr4),
    REG(NULL,   "r5",  NULL,       Gpr5),
    REG(NULL,   "r6",  NULL,       Gpr6),
    REG(NULL,   "r7",  NULL,       Gpr7),
    REG(NULL,   "r8",  NULL,       Gpr8),
    REG(NULL,   "r9",  NULL,       Gpr9),
    REG(NULL,   "r10", NULL,       Gpr10),
    REG(NULL,   "r11", NULL,       Gpr11),
    REG(NULL,   "r12", NULL,       Gpr12),
    REG(NULL,   "r13", NULL,       Gpr13),
    REG(NULL,   "r14", NULL,       Gpr14),
    REG(NULL,   "r15", NULL,       Gpr15),
    REG(NULL,   "r16", NULL,       Gpr16),
    REG(NULL,   "r17", NULL,       Gpr17),
    REG(NULL,   "r18", NULL,       Gpr18),
    REG(NULL,   "r19", NULL,       Gpr19),
    REG(NULL,   "r20", NULL,       Gpr20),
    REG(NULL,   "r21", NULL,       Gpr21),
    REG(NULL,   "r22", NULL,       Gpr22),
    REG(NULL,   "r23", NULL,       Gpr23),
    REG(NULL,   "r24", NULL,       Gpr24),
    REG(NULL,   "r25", NULL,       Gpr25),
    REG(NULL,   "r26", NULL,       Gpr26),
    REG(NULL,   "r27", NULL,       Gpr27),
    REG(NULL,   "r28", NULL,       Gpr28),
    REG(NULL,   "r29", NULL,       Gpr29),
    REG(NULL,   "r30", NULL,       Gpr30),
    REG(NULL,   "r31", NULL,       Gpr31),
    REG(NULL,   "pc",  "code_ptr", Iar),
    REG(NULL,   "msr", NULL,       Msr),
    REG(NULL,   "cr",  NULL,       Cr),
    REG(NULL,   "lr",  "code_ptr", Lr),
    REG(NULL,   "ctr", NULL,       Ctr),
    REG(NULL,   "xer", NULL,       Xer),

    REG("fpu", "f0",    "ieee_single", Fpr0,  4),
    REG(NULL,  "f1",    "ieee_single", Fpr1,  4),
    REG(NULL,  "f2",    "ieee_single", Fpr2,  4),
    REG(NULL,  "f3",    "ieee_single", Fpr3,  4),
    REG(NULL,  "f4",    "ieee_single", Fpr4,  4),
    REG(NULL,  "f5",    "ieee_single", Fpr5,  4),
    REG(NULL,  "f6",    "ieee_single", Fpr6,  4),
    REG(NULL,  "f7",    "ieee_single", Fpr7,  4),
    REG(NULL,  "f8",    "ieee_single", Fpr8,  4),
    REG(NULL,  "f9",    "ieee_single", Fpr9,  4),
    REG(NULL,  "f10",   "ieee_single", Fpr10, 4),
    REG(NULL,  "f11",   "ieee_single", Fpr11, 4),
    REG(NULL,  "f12",   "ieee_single", Fpr12, 4),
    REG(NULL,  "f13",   "ieee_single", Fpr13, 4),
    REG(NULL,  "f14",   "ieee_single", Fpr14, 4),
    REG(NULL,  "f15",   "ieee_single", Fpr15, 4),
    REG(NULL,  "f16",   "ieee_single", Fpr16, 4),
    REG(NULL,  "f17",   "ieee_single", Fpr17, 4),
    REG(NULL,  "f18",   "ieee_single", Fpr18, 4),
    REG(NULL,  "f19",   "ieee_single", Fpr19, 4),
    REG(NULL,  "f20",   "ieee_single", Fpr20, 4),
    REG(NULL,  "f21",   "ieee_single", Fpr21, 4),
    REG(NULL,  "f22",   "ieee_single", Fpr22, 4),
    REG(NULL,  "f23",   "ieee_single", Fpr23, 4),
    REG(NULL,  "f24",   "ieee_single", Fpr24, 4),
    REG(NULL,  "f25",   "ieee_single", Fpr25, 4),
    REG(NULL,  "f26",   "ieee_single", Fpr26, 4),
    REG(NULL,  "f27",   "ieee_single", Fpr27, 4),
    REG(NULL,  "f28",   "ieee_single", Fpr28, 4),
    REG(NULL,  "f29",   "ieee_single", Fpr29, 4),
    REG(NULL,  "f30",   "ieee_single", Fpr30, 4),
    REG(NULL,  "f31",   "ieee_single", Fpr31, 4),
    REG(NULL,  "fpscr", NULL,          Fpscr, 4),

    /* FIXME: MQ is missing? FIELD_OFFSET(CONTEXT, Mq), */
    /* see gdb/nlm/ppc.c */
};

struct backend_cpu be_ppc =
{
    IMAGE_FILE_MACHINE_POWERPC,
    4,
    be_cpu_linearize,
    be_cpu_build_addr,
    be_ppc_get_addr,
    be_ppc_get_register_info,
    be_ppc_single_step,
    be_ppc_print_context,
    be_ppc_print_segment_info,
    be_ppc_ctx,
    be_ppc_is_step_over_insn,
    be_ppc_is_function_return,
    be_ppc_is_break_insn,
    be_ppc_is_func_call,
    be_ppc_is_jump,
    be_ppc_disasm_one_insn,
    be_ppc_insert_Xpoint,
    be_ppc_remove_Xpoint,
    be_ppc_is_watchpoint_set,
    be_ppc_clear_watchpoint,
    be_ppc_adjust_pc_for_break,
    be_ppc_fetch_integer,
    be_ppc_fetch_float,
    be_ppc_store_integer,
    be_ppc_get_context,
    be_ppc_set_context,
    be_ppc_gdb_register_map,
    ARRAY_SIZE(be_ppc_gdb_register_map),
};
#endif
