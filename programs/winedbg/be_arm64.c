/*
 * Debugger ARM64 specific functions
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

#if defined(__aarch64__) && !defined(__AARCH64EB__)

static BOOL be_arm64_get_addr(HANDLE hThread, const dbg_ctx_t *ctx,
                              enum be_cpu_addr bca, ADDRESS64* addr)
{
    switch (bca)
    {
    case be_cpu_addr_pc:
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->ctx.Pc);
    case be_cpu_addr_stack:
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->ctx.Sp);
    case be_cpu_addr_frame:
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->ctx.Fp);
        break;
    }
    return FALSE;
}

static BOOL be_arm64_get_register_info(int regno, enum be_cpu_addr* kind)
{
    switch (regno)
    {
    case CV_ARM64_PC:      *kind = be_cpu_addr_pc;    return TRUE;
    case CV_ARM64_SP:      *kind = be_cpu_addr_stack; return TRUE;
    case CV_ARM64_FP:      *kind = be_cpu_addr_frame; return TRUE;
    }
    return FALSE;
}

static void be_arm64_single_step(dbg_ctx_t *ctx, BOOL enable)
{
    dbg_printf("be_arm64_single_step: not done\n");
}

static void be_arm64_print_context(HANDLE hThread, const dbg_ctx_t *ctx, int all_regs)
{
    static const char condflags[] = "NZCV";
    int i;
    char        buf[8];

    switch (ctx->ctx.Cpsr & 0x0f)
    {
    case 0:  strcpy(buf, "EL0t"); break;
    case 4:  strcpy(buf, "EL1t"); break;
    case 5:  strcpy(buf, "EL1t"); break;
    case 8:  strcpy(buf, "EL2t"); break;
    case 9:  strcpy(buf, "EL2t"); break;
    case 12: strcpy(buf, "EL3t"); break;
    case 13: strcpy(buf, "EL3t"); break;
    default: strcpy(buf, "UNKNWN"); break;
    }

    dbg_printf("Register dump:\n");
    dbg_printf("%s %s Mode\n", (ctx->ctx.Cpsr & 0x10) ? "ARM" : "ARM64", buf);

    strcpy(buf, condflags);
    for (i = 0; buf[i]; i++)
        if (!((ctx->ctx.Cpsr >> 26) & (1 << (sizeof(condflags) - i))))
            buf[i] = '-';

    dbg_printf(" Pc:%016I64x Sp:%016I64x Lr:%016I64x Cpsr:%08lx(%s)\n",
               ctx->ctx.Pc, ctx->ctx.Sp, ctx->ctx.Lr, ctx->ctx.Cpsr, buf);
    dbg_printf(" x0: %016I64x x1: %016I64x x2: %016I64x x3: %016I64x x4: %016I64x\n",
               ctx->ctx.X0, ctx->ctx.X1, ctx->ctx.X2, ctx->ctx.X3, ctx->ctx.X4);
    dbg_printf(" x5: %016I64x x6: %016I64x x7: %016I64x x8: %016I64x x9: %016I64x\n",
               ctx->ctx.X5, ctx->ctx.X6, ctx->ctx.X7, ctx->ctx.X8, ctx->ctx.X9);
    dbg_printf(" x10:%016I64x x11:%016I64x x12:%016I64x x13:%016I64x x14:%016I64x\n",
               ctx->ctx.X10, ctx->ctx.X11, ctx->ctx.X12, ctx->ctx.X13, ctx->ctx.X14);
    dbg_printf(" x15:%016I64x ip0:%016I64x ip1:%016I64x x18:%016I64x x19:%016I64x\n",
               ctx->ctx.X15, ctx->ctx.X16, ctx->ctx.X17, ctx->ctx.X18, ctx->ctx.X19);
    dbg_printf(" x20:%016I64x x21:%016I64x x22:%016I64x x23:%016I64x x24:%016I64x\n",
               ctx->ctx.X20, ctx->ctx.X21, ctx->ctx.X22, ctx->ctx.X23, ctx->ctx.X24);
    dbg_printf(" x25:%016I64x x26:%016I64x x27:%016I64x x28:%016I64x Fp:%016I64x\n",
               ctx->ctx.X25, ctx->ctx.X26, ctx->ctx.X27, ctx->ctx.X28, ctx->ctx.Fp);

    if (all_regs) dbg_printf( "Floating point ARM64 dump not implemented\n" );
}

static void be_arm64_print_segment_info(HANDLE hThread, const dbg_ctx_t *ctx)
{
}

static struct dbg_internal_var be_arm64_ctx[] =
{
    {CV_ARM64_PSTATE,     "cpsr",   (void*)FIELD_OFFSET(CONTEXT, Cpsr), dbg_itype_unsigned_int32},
    {CV_ARM64_X0 +  0,    "x0",     (void*)FIELD_OFFSET(CONTEXT, X0),   dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  1,    "x1",     (void*)FIELD_OFFSET(CONTEXT, X1),   dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  2,    "x2",     (void*)FIELD_OFFSET(CONTEXT, X2),   dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  3,    "x3",     (void*)FIELD_OFFSET(CONTEXT, X3),   dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  4,    "x4",     (void*)FIELD_OFFSET(CONTEXT, X4),   dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  5,    "x5",     (void*)FIELD_OFFSET(CONTEXT, X5),   dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  6,    "x6",     (void*)FIELD_OFFSET(CONTEXT, X6),   dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  7,    "x7",     (void*)FIELD_OFFSET(CONTEXT, X7),   dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  8,    "x8",     (void*)FIELD_OFFSET(CONTEXT, X8),   dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  9,    "x9",     (void*)FIELD_OFFSET(CONTEXT, X9),   dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  10,   "x10",    (void*)FIELD_OFFSET(CONTEXT, X10),  dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  11,   "x11",    (void*)FIELD_OFFSET(CONTEXT, X11),  dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  12,   "x12",    (void*)FIELD_OFFSET(CONTEXT, X12),  dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  13,   "x13",    (void*)FIELD_OFFSET(CONTEXT, X13),  dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  14,   "x14",    (void*)FIELD_OFFSET(CONTEXT, X14),  dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  15,   "x15",    (void*)FIELD_OFFSET(CONTEXT, X15),  dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  16,   "x16",    (void*)FIELD_OFFSET(CONTEXT, X16),  dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  17,   "x17",    (void*)FIELD_OFFSET(CONTEXT, X17),  dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  18,   "x18",    (void*)FIELD_OFFSET(CONTEXT, X18),  dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  19,   "x19",    (void*)FIELD_OFFSET(CONTEXT, X19),  dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  20,   "x20",    (void*)FIELD_OFFSET(CONTEXT, X20),  dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  21,   "x21",    (void*)FIELD_OFFSET(CONTEXT, X21),  dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  22,   "x22",    (void*)FIELD_OFFSET(CONTEXT, X22),  dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  23,   "x23",    (void*)FIELD_OFFSET(CONTEXT, X23),  dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  24,   "x24",    (void*)FIELD_OFFSET(CONTEXT, X24),  dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  25,   "x25",    (void*)FIELD_OFFSET(CONTEXT, X25),  dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  26,   "x26",    (void*)FIELD_OFFSET(CONTEXT, X26),  dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  27,   "x27",    (void*)FIELD_OFFSET(CONTEXT, X27),  dbg_itype_unsigned_int64},
    {CV_ARM64_X0 +  28,   "x28",    (void*)FIELD_OFFSET(CONTEXT, X28),  dbg_itype_unsigned_int64},
    {CV_ARM64_FP,         "fp",     (void*)FIELD_OFFSET(CONTEXT, Fp),   dbg_itype_unsigned_int64},
    {CV_ARM64_LR,         "lr",     (void*)FIELD_OFFSET(CONTEXT, Lr),   dbg_itype_unsigned_int64},
    {CV_ARM64_SP,         "sp",     (void*)FIELD_OFFSET(CONTEXT, Sp),   dbg_itype_unsigned_int64},
    {CV_ARM64_PC,         "pc",     (void*)FIELD_OFFSET(CONTEXT, Pc),   dbg_itype_unsigned_int64},
    {0,                   NULL,     0,                                  dbg_itype_none}
};

static BOOL be_arm64_is_step_over_insn(const void* insn)
{
    dbg_printf("be_arm64_is_step_over_insn: not done\n");
    return FALSE;
}

static BOOL be_arm64_is_function_return(const void* insn)
{
    dbg_printf("be_arm64_is_function_return: not done\n");
    return FALSE;
}

static BOOL be_arm64_is_break_insn(const void* insn)
{
    dbg_printf("be_arm64_is_break_insn: not done\n");
    return FALSE;
}

static BOOL be_arm64_is_func_call(const void* insn, ADDRESS64* callee)
{
    return FALSE;
}

static BOOL be_arm64_is_jump(const void* insn, ADDRESS64* jumpee)
{
    return FALSE;
}

static BOOL be_arm64_insert_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                   dbg_ctx_t *ctx, enum be_xpoint_type type,
                                   void* addr, unsigned *val, unsigned size)
{
    SIZE_T              sz;

    switch (type)
    {
    case be_xpoint_break:
        if (!size) return FALSE;
        if (!pio->read(hProcess, addr, val, 4, &sz) || sz != 4) return FALSE;
    default:
        dbg_printf("Unknown/unsupported bp type %c\n", type);
        return FALSE;
    }
    return TRUE;
}

static BOOL be_arm64_remove_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                   dbg_ctx_t *ctx, enum be_xpoint_type type,
                                   void* addr, unsigned val, unsigned size)
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

static BOOL be_arm64_is_watchpoint_set(const dbg_ctx_t *ctx, unsigned idx)
{
    dbg_printf("be_arm64_is_watchpoint_set: not done\n");
    return FALSE;
}

static void be_arm64_clear_watchpoint(dbg_ctx_t *ctx, unsigned idx)
{
    dbg_printf("be_arm64_clear_watchpoint: not done\n");
}

static int be_arm64_adjust_pc_for_break(dbg_ctx_t *ctx, BOOL way)
{
    if (way)
    {
        ctx->ctx.Pc -= 4;
        return -4;
    }
    ctx->ctx.Pc += 4;
    return 4;
}

void be_arm64_disasm_one_insn(ADDRESS64 *addr, int display)
{
    dbg_printf("be_arm64_disasm_one_insn: not done\n");
}

static BOOL be_arm64_get_context(HANDLE thread, dbg_ctx_t *ctx)
{
    ctx->ctx.ContextFlags = CONTEXT_ALL;
    return GetThreadContext(thread, &ctx->ctx);
}

static BOOL be_arm64_set_context(HANDLE thread, const dbg_ctx_t *ctx)
{
    return SetThreadContext(thread, &ctx->ctx);
}

#define REG(f,n,t,r)  {f, n, t, FIELD_OFFSET(CONTEXT, r), sizeof(((CONTEXT*)NULL)->r)}

static struct gdb_register be_arm64_gdb_register_map[] = {
    REG("core", "x0",   NULL,         X0),
    REG(NULL,   "x1",   NULL,         X1),
    REG(NULL,   "x2",   NULL,         X2),
    REG(NULL,   "x3",   NULL,         X3),
    REG(NULL,   "x4",   NULL,         X4),
    REG(NULL,   "x5",   NULL,         X5),
    REG(NULL,   "x6",   NULL,         X6),
    REG(NULL,   "x7",   NULL,         X7),
    REG(NULL,   "x8",   NULL,         X8),
    REG(NULL,   "x9",   NULL,         X9),
    REG(NULL,   "x10",  NULL,         X10),
    REG(NULL,   "x11",  NULL,         X11),
    REG(NULL,   "x12",  NULL,         X12),
    REG(NULL,   "x13",  NULL,         X13),
    REG(NULL,   "x14",  NULL,         X14),
    REG(NULL,   "x15",  NULL,         X15),
    REG(NULL,   "x16",  NULL,         X16),
    REG(NULL,   "x17",  NULL,         X17),
    REG(NULL,   "x18",  NULL,         X18),
    REG(NULL,   "x19",  NULL,         X19),
    REG(NULL,   "x20",  NULL,         X20),
    REG(NULL,   "x21",  NULL,         X21),
    REG(NULL,   "x22",  NULL,         X22),
    REG(NULL,   "x23",  NULL,         X23),
    REG(NULL,   "x24",  NULL,         X24),
    REG(NULL,   "x25",  NULL,         X25),
    REG(NULL,   "x26",  NULL,         X26),
    REG(NULL,   "x27",  NULL,         X27),
    REG(NULL,   "x28",  NULL,         X28),
    REG(NULL,   "x29",  NULL,         Fp),
    REG(NULL,   "x30",  NULL,         Lr),
    REG(NULL,   "sp",   "data_ptr",   Sp),
    REG(NULL,   "pc",   "code_ptr",   Pc),
    REG(NULL,   "cpsr", "cpsr_flags", Cpsr),
};

struct backend_cpu be_arm64 =
{
    IMAGE_FILE_MACHINE_ARM64,
    8,
    be_cpu_linearize,
    be_cpu_build_addr,
    be_arm64_get_addr,
    be_arm64_get_register_info,
    be_arm64_single_step,
    be_arm64_print_context,
    be_arm64_print_segment_info,
    be_arm64_ctx,
    be_arm64_is_step_over_insn,
    be_arm64_is_function_return,
    be_arm64_is_break_insn,
    be_arm64_is_func_call,
    be_arm64_is_jump,
    be_arm64_disasm_one_insn,
    be_arm64_insert_Xpoint,
    be_arm64_remove_Xpoint,
    be_arm64_is_watchpoint_set,
    be_arm64_clear_watchpoint,
    be_arm64_adjust_pc_for_break,
    be_arm64_get_context,
    be_arm64_set_context,
    be_arm64_gdb_register_map,
    ARRAY_SIZE(be_arm64_gdb_register_map),
};
#endif
