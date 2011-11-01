/*
 * Debugger ARM specific functions
 *
 * Copyright 2000-2003 Marcus Meissner
 *                2004 Eric Pouech
 *                2010, 2011 André Hentschel
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

#if defined(__arm__)

static unsigned be_arm_get_addr(HANDLE hThread, const CONTEXT* ctx,
                                enum be_cpu_addr bca, ADDRESS64* addr)
{
    switch (bca)
    {
    case be_cpu_addr_pc:
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->Pc);
    case be_cpu_addr_stack:
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->Sp);
    case be_cpu_addr_frame:
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->Fp);
    }
    return FALSE;
}

static unsigned be_arm_get_register_info(int regno, enum be_cpu_addr* kind)
{
    switch (regno)
    {
    case CV_ARM_PC:  *kind = be_cpu_addr_pc; return TRUE;
    case CV_ARM_R0 + 11: *kind = be_cpu_addr_frame; return TRUE;
    case CV_ARM_SP:  *kind = be_cpu_addr_stack; return TRUE;
    }
    return FALSE;
}

static void be_arm_single_step(CONTEXT* ctx, unsigned enable)
{
    dbg_printf("be_arm_single_step: not done\n");
}

static void be_arm_print_context(HANDLE hThread, const CONTEXT* ctx, int all_regs)
{
    static const char condflags[] = "NZCV";
    int i;
    char        buf[8];

    switch (ctx->Cpsr & 0x1F)
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
    dbg_printf("%s %s Mode\n", (ctx->Cpsr & 0x20) ? "Thumb" : "ARM", buf);

    strcpy(buf, condflags);
    for (i = 0; buf[i]; i++)
        if (!((ctx->Cpsr >> 26) & (1 << (sizeof(condflags) - i))))
            buf[i] = '-';

    dbg_printf(" Pc:%04x Sp:%04x Lr:%04x Cpsr:%04x(%s)\n",
               ctx->Pc, ctx->Sp, ctx->Lr, ctx->Cpsr, buf);
    dbg_printf(" r0:%04x r1:%04x r2:%04x r3:%04x\n",
               ctx->R0, ctx->R1, ctx->R2, ctx->R3);
    dbg_printf(" r4:%04x r5:%04x  r6:%04x  r7:%04x r8:%04x\n",
               ctx->R4, ctx->R5, ctx->R6, ctx->R7, ctx->R8 );
    dbg_printf(" r9:%04x r10:%04x Fp:%04x Ip:%04x\n",
               ctx->R9, ctx->R10, ctx->Fp, ctx->Ip );

    if (all_regs) dbg_printf( "Floating point ARM dump not implemented\n" );
}

static void be_arm_print_segment_info(HANDLE hThread, const CONTEXT* ctx)
{
}

static struct dbg_internal_var be_arm_ctx[] =
{
    {CV_ARM_R0 +  0,    "r0",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R0),     dbg_itype_unsigned_int},
    {CV_ARM_R0 +  1,    "r1",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R1),     dbg_itype_unsigned_int},
    {CV_ARM_R0 +  2,    "r2",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R2),     dbg_itype_unsigned_int},
    {CV_ARM_R0 +  3,    "r3",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R3),     dbg_itype_unsigned_int},
    {CV_ARM_R0 +  4,    "r4",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R4),     dbg_itype_unsigned_int},
    {CV_ARM_R0 +  5,    "r5",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R5),     dbg_itype_unsigned_int},
    {CV_ARM_R0 +  6,    "r6",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R6),     dbg_itype_unsigned_int},
    {CV_ARM_R0 +  7,    "r7",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R7),     dbg_itype_unsigned_int},
    {CV_ARM_R0 +  8,    "r8",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R8),     dbg_itype_unsigned_int},
    {CV_ARM_R0 +  9,    "r9",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R9),     dbg_itype_unsigned_int},
    {CV_ARM_R0 +  10,   "r10",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R10),    dbg_itype_unsigned_int},
    {CV_ARM_R0 +  11,   "r11",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Fp),     dbg_itype_unsigned_int},
    {CV_ARM_R0 +  12,   "r12",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Ip),     dbg_itype_unsigned_int},
    {CV_ARM_SP,         "sp",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Sp),     dbg_itype_unsigned_int},
    {CV_ARM_LR,         "lr",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Lr),     dbg_itype_unsigned_int},
    {CV_ARM_PC,         "pc",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Pc),     dbg_itype_unsigned_int},
    {CV_ARM_CPSR,       "cpsr",         (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Cpsr),   dbg_itype_unsigned_int},
    {0,                 NULL,           0,                                         dbg_itype_none}
};

static unsigned be_arm_is_step_over_insn(const void* insn)
{
    dbg_printf("be_arm_is_step_over_insn: not done\n");
    return FALSE;
}

static unsigned be_arm_is_function_return(const void* insn)
{
    dbg_printf("be_arm_is_function_return: not done\n");
    return FALSE;
}

static unsigned be_arm_is_break_insn(const void* insn)
{
    dbg_printf("be_arm_is_break_insn: not done\n");
    return FALSE;
}

static unsigned be_arm_is_func_call(const void* insn, ADDRESS64* callee)
{
    return FALSE;
}

static unsigned be_arm_is_jump(const void* insn, ADDRESS64* jumpee)
{
    return FALSE;
}

static void be_arm_disasm_one_insn(ADDRESS64* addr, int display)
{
    dbg_printf("Disasm NIY\n");
}

static unsigned be_arm_insert_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                     CONTEXT* ctx, enum be_xpoint_type type,
                                     void* addr, unsigned long* val, unsigned size)
{
    SIZE_T              sz;

    switch (type)
    {
    case be_xpoint_break:
        if (!size) return 0;
        if (!pio->read(hProcess, addr, val, 4, &sz) || sz != 4) return 0;
    default:
        dbg_printf("Unknown/unsupported bp type %c\n", type);
        return 0;
    }
    return 1;
}

static unsigned be_arm_remove_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                     CONTEXT* ctx, enum be_xpoint_type type,
                                     void* addr, unsigned long val, unsigned size)
{
    SIZE_T              sz;

    switch (type)
    {
    case be_xpoint_break:
        if (!size) return 0;
        if (!pio->write(hProcess, addr, &val, 4, &sz) || sz == 4) return 0;
        break;
    default:
        dbg_printf("Unknown/unsupported bp type %c\n", type);
        return 0;
    }
    return 1;
}

static unsigned be_arm_is_watchpoint_set(const CONTEXT* ctx, unsigned idx)
{
    dbg_printf("be_arm_is_watchpoint_set: not done\n");
    return FALSE;
}

static void be_arm_clear_watchpoint(CONTEXT* ctx, unsigned idx)
{
    dbg_printf("be_arm_clear_watchpoint: not done\n");
}

static int be_arm_adjust_pc_for_break(CONTEXT* ctx, BOOL way)
{
    if (way)
    {
        ctx->Pc-=4;
        return -4;
    }
    ctx->Pc+=4;
    return 4;
}

static int be_arm_fetch_integer(const struct dbg_lvalue* lvalue, unsigned size,
                                unsigned ext_sign, LONGLONG* ret)
{
    if (size != 1 && size != 2 && size != 4 && size != 8) return FALSE;

    memset(ret, 0, sizeof(*ret)); /* clear unread bytes */
    /* FIXME: this assumes that debuggee and debugger use the same
     * integral representation
     */
    if (!memory_read_value(lvalue, size, ret)) return FALSE;

    /* propagate sign information */
    if (ext_sign && size < 8 && (*ret >> (size * 8 - 1)) != 0)
    {
        ULONGLONG neg = -1;
        *ret |= neg << (size * 8);
    }
    return TRUE;
}

static int be_arm_fetch_float(const struct dbg_lvalue* lvalue, unsigned size,
                              long double* ret)
{
    dbg_printf("be_arm_fetch_float: not done\n");
    return FALSE;
}

struct backend_cpu be_arm =
{
    IMAGE_FILE_MACHINE_ARMV7,
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
    be_arm_fetch_integer,
    be_arm_fetch_float,
};
#endif
