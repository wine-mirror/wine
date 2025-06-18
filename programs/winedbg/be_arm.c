/*
 * Debugger ARM specific functions
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

#ifdef __arm__

#include <capstone/capstone.h>

extern cs_opt_mem cs_mem;

/***********************************************************************
 *              disasm_one_insn
 *
 * Disassemble instruction at 'addr'. addr is changed to point to the
 * start of the next instruction.
 */
void be_arm_disasm_one_insn(ADDRESS64 *addr, int display)
{
    static csh handle;
    cs_insn *insn;
    unsigned char buffer[4];
    SIZE_T count, len;
    DWORD pc = addr->Offset;

    addr->Offset &= ~1;
    if (!dbg_curr_process->process_io->read( dbg_curr_process->handle, memory_to_linear_addr(addr),
                                             buffer, sizeof(buffer), &len )) return;

    if (!handle)
    {
        cs_option( 0, CS_OPT_MEM, (size_t)&cs_mem );
        cs_open( CS_ARCH_ARM, 0, &handle );
    }
    cs_option( handle, CS_OPT_MODE, pc & 1 ? CS_MODE_THUMB : CS_MODE_ARM );
    cs_option( handle, CS_OPT_DETAIL, CS_OPT_ON );
    count = cs_disasm( handle, buffer, len, pc, 0, &insn );
    if (display) dbg_printf( "%s %s", insn[0].mnemonic, insn[0].op_str );
    if (insn[0].id == ARM_INS_B || insn[0].id == ARM_INS_BL)
    {
        ADDRESS64 a = { .Mode = AddrModeFlat, .Offset = insn[0].detail->arm.operands[0].imm };
        print_address_symbol( &a, TRUE, "" );
    }
    addr->Offset = pc + insn[0].size;
    cs_free( insn, count );
}

static BOOL be_arm_get_addr(HANDLE hThread, const dbg_ctx_t *ctx,
                            enum be_cpu_addr bca, ADDRESS64* addr)
{
    switch (bca)
    {
    case be_cpu_addr_pc:
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->ctx.Pc);
    case be_cpu_addr_stack:
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->ctx.Sp);
    case be_cpu_addr_frame:
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->ctx.R11);
    }
    return FALSE;
}

static BOOL be_arm_get_register_info(int regno, enum be_cpu_addr* kind)
{
    switch (regno)
    {
    case CV_ARM_PC:  *kind = be_cpu_addr_pc; return TRUE;
    case CV_ARM_R0 + 11: *kind = be_cpu_addr_frame; return TRUE;
    case CV_ARM_SP:  *kind = be_cpu_addr_stack; return TRUE;
    }
    return FALSE;
}

static void be_arm_single_step(dbg_ctx_t *ctx, BOOL enable)
{
}

static void be_arm_print_context(HANDLE hThread, const dbg_ctx_t *ctx, int all_regs)
{
    static const char condflags[] = "NZCV";
    int i;
    char        buf[8];

    switch (ctx->ctx.Cpsr & 0x1F)
    {
    case 0:  strcpy(buf, "User26"); break;
    case 1:  strcpy(buf, "FIQ26"); break;
    case 2:  strcpy(buf, "IRQ26"); break;
    case 3:  strcpy(buf, "SVC26"); break;
    case 16: strcpy(buf, "User"); break;
    case 17: strcpy(buf, "FIQ"); break;
    case 18: strcpy(buf, "IRQ"); break;
    case 19: strcpy(buf, "SVC"); break;
    case 23: strcpy(buf, "ABT"); break;
    case 27: strcpy(buf, "UND"); break;
    default: strcpy(buf, "UNKNWN"); break;
    }

    dbg_printf("Register dump:\n");
    dbg_printf("%s %s Mode\n", (ctx->ctx.Cpsr & 0x20) ? "Thumb" : "ARM", buf);

    strcpy(buf, condflags);
    for (i = 0; buf[i]; i++)
        if (!((ctx->ctx.Cpsr >> 26) & (1 << (sizeof(condflags) - i))))
            buf[i] = '-';

    dbg_printf(" Pc:%08lx Sp:%08lx Lr:%08lx Cpsr:%08lx(%s)\n",
               ctx->ctx.Pc, ctx->ctx.Sp, ctx->ctx.Lr, ctx->ctx.Cpsr, buf);
    dbg_printf(" r0:%08lx r1:%08lx r2:%08lx r3:%08lx\n",
               ctx->ctx.R0, ctx->ctx.R1, ctx->ctx.R2, ctx->ctx.R3);
    dbg_printf(" r4:%08lx r5:%08lx r6:%08lx r7:%08lx\n",
               ctx->ctx.R4, ctx->ctx.R5, ctx->ctx.R6, ctx->ctx.R7);
    dbg_printf(" r8:%08lx r9:%08lx r10:%08lx r11:%08lx r12:%08lx\n",
               ctx->ctx.R8, ctx->ctx.R9, ctx->ctx.R10, ctx->ctx.R11, ctx->ctx.R12);

    if (all_regs) dbg_printf( "Floating point ARM dump not implemented\n" );
}

static void be_arm_print_segment_info(HANDLE hThread, const dbg_ctx_t *ctx)
{
}

static struct dbg_internal_var be_arm_ctx[] =
{
    {CV_ARM_R0 +  0,    "r0",           (void*)FIELD_OFFSET(CONTEXT, R0),     dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  1,    "r1",           (void*)FIELD_OFFSET(CONTEXT, R1),     dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  2,    "r2",           (void*)FIELD_OFFSET(CONTEXT, R2),     dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  3,    "r3",           (void*)FIELD_OFFSET(CONTEXT, R3),     dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  4,    "r4",           (void*)FIELD_OFFSET(CONTEXT, R4),     dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  5,    "r5",           (void*)FIELD_OFFSET(CONTEXT, R5),     dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  6,    "r6",           (void*)FIELD_OFFSET(CONTEXT, R6),     dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  7,    "r7",           (void*)FIELD_OFFSET(CONTEXT, R7),     dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  8,    "r8",           (void*)FIELD_OFFSET(CONTEXT, R8),     dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  9,    "r9",           (void*)FIELD_OFFSET(CONTEXT, R9),     dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  10,   "r10",          (void*)FIELD_OFFSET(CONTEXT, R10),    dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  11,   "r11",          (void*)FIELD_OFFSET(CONTEXT, R11),    dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  12,   "r12",          (void*)FIELD_OFFSET(CONTEXT, R12),    dbg_itype_unsigned_int32},
    {CV_ARM_SP,         "sp",           (void*)FIELD_OFFSET(CONTEXT, Sp),     dbg_itype_unsigned_int32},
    {CV_ARM_LR,         "lr",           (void*)FIELD_OFFSET(CONTEXT, Lr),     dbg_itype_unsigned_int32},
    {CV_ARM_PC,         "pc",           (void*)FIELD_OFFSET(CONTEXT, Pc),     dbg_itype_unsigned_int32},
    {CV_ARM_CPSR,       "cpsr",         (void*)FIELD_OFFSET(CONTEXT, Cpsr),   dbg_itype_unsigned_int32},
    {0,                 NULL,           0,                                    dbg_itype_none}
};

static BOOL be_arm_is_step_over_insn(const void* insn)
{
    dbg_printf("be_arm_is_step_over_insn: not done\n");
    return FALSE;
}

static BOOL be_arm_is_function_return(const void* insn)
{
    dbg_printf("be_arm_is_function_return: not done\n");
    return FALSE;
}

static BOOL be_arm_is_break_insn(const void* insn)
{
    dbg_printf("be_arm_is_break_insn: not done\n");
    return FALSE;
}

static BOOL be_arm_is_func_call(const void* insn, ADDRESS64* callee)
{
    return FALSE;
}

static BOOL be_arm_is_jump(const void* insn, ADDRESS64* jumpee)
{
    return FALSE;
}

static BOOL be_arm_insert_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
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

static BOOL be_arm_remove_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
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

static BOOL be_arm_is_watchpoint_set(const dbg_ctx_t *ctx, unsigned idx)
{
    dbg_printf("be_arm_is_watchpoint_set: not done\n");
    return FALSE;
}

static void be_arm_clear_watchpoint(dbg_ctx_t *ctx, unsigned idx)
{
    dbg_printf("be_arm_clear_watchpoint: not done\n");
}

static int be_arm_adjust_pc_for_break(dbg_ctx_t *ctx, BOOL way)
{
    INT step = (ctx->ctx.Cpsr & 0x20) ? 2 : 4;

    if (way)
    {
        ctx->ctx.Pc -= step;
        return -step;
    }
    ctx->ctx.Pc += step;
    return step;
}

static BOOL be_arm_get_context(HANDLE thread, dbg_ctx_t *ctx)
{
    ctx->ctx.ContextFlags = CONTEXT_ALL;
    return GetThreadContext(thread, &ctx->ctx);
}

static BOOL be_arm_set_context(HANDLE thread, const dbg_ctx_t *ctx)
{
    return SetThreadContext(thread, &ctx->ctx);
}

#define REG(f,n,t,r)  {f, n, t, FIELD_OFFSET(CONTEXT, r), sizeof(((CONTEXT*)NULL)->r)}

static struct gdb_register be_arm_gdb_register_map[] = {
    REG("core", "r0",   NULL,       R0),
    REG(NULL,   "r1",   NULL,       R1),
    REG(NULL,   "r2",   NULL,       R2),
    REG(NULL,   "r3",   NULL,       R3),
    REG(NULL,   "r4",   NULL,       R4),
    REG(NULL,   "r5",   NULL,       R5),
    REG(NULL,   "r6",   NULL,       R6),
    REG(NULL,   "r7",   NULL,       R7),
    REG(NULL,   "r8",   NULL,       R8),
    REG(NULL,   "r9",   NULL,       R9),
    REG(NULL,   "r10",  NULL,       R10),
    REG(NULL,   "r11",  NULL,       R11),
    REG(NULL,   "r12",  NULL,       R12),
    REG(NULL,   "sp",   "data_ptr", Sp),
    REG(NULL,   "lr",   "code_ptr", Lr),
    REG(NULL,   "pc",   "code_ptr", Pc),
    REG(NULL,   "cpsr", NULL,       Cpsr),
};

struct backend_cpu be_arm =
{
    IMAGE_FILE_MACHINE_ARMNT,
    4,
    be_cpu_linearize,
    be_cpu_build_addr,
    be_arm_get_addr,
    be_arm_get_register_info,
    be_arm_single_step,
    be_arm_print_context,
    be_arm_print_segment_info,
    be_arm_ctx,
    be_arm_is_step_over_insn,
    be_arm_is_function_return,
    be_arm_is_break_insn,
    be_arm_is_func_call,
    be_arm_is_jump,
    be_arm_disasm_one_insn,
    be_arm_insert_Xpoint,
    be_arm_remove_Xpoint,
    be_arm_is_watchpoint_set,
    be_arm_clear_watchpoint,
    be_arm_adjust_pc_for_break,
    be_arm_get_context,
    be_arm_set_context,
    be_arm_gdb_register_map,
    ARRAY_SIZE(be_arm_gdb_register_map),
};
#endif
