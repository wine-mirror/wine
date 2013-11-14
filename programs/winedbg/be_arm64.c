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

static BOOL be_arm64_get_addr(HANDLE hThread, const CONTEXT* ctx,
                              enum be_cpu_addr bca, ADDRESS64* addr)
{
    switch (bca)
    {
    case be_cpu_addr_pc:
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->Pc);
    case be_cpu_addr_stack:
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->Sp);
    case be_cpu_addr_frame:
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->X29);
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
    case CV_ARM64_X0 + 29: *kind = be_cpu_addr_frame; return TRUE;
    }
    return FALSE;
}

static void be_arm64_single_step(CONTEXT* ctx, BOOL enable)
{
    dbg_printf("be_arm64_single_step: not done\n");
}

static void be_arm64_print_context(HANDLE hThread, const CONTEXT* ctx, int all_regs)
{
    static const char condflags[] = "NZCV";
    int i;
    char        buf[8];

    switch (ctx->PState & 0x0f)
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
    dbg_printf("%s %s Mode\n", (ctx->PState & 0x10) ? "ARM" : "ARM64", buf);

    strcpy(buf, condflags);
    for (i = 0; buf[i]; i++)
        if (!((ctx->PState >> 26) & (1 << (sizeof(condflags) - i))))
            buf[i] = '-';

    dbg_printf(" Pc:%016lx Sp:%016lx Pstate:%016lx(%s)\n",
               ctx->Pc, ctx->Sp, ctx->PState, buf);
    dbg_printf(" x0: %016lx x1: %016lx x2: %016lx x3: %016lx x4: %016lx\n",
               ctx->X0, ctx->X1, ctx->X2, ctx->X3, ctx->X4);
    dbg_printf(" x5: %016lx x6: %016lx x7: %016lx x8: %016lx x9: %016lx\n",
               ctx->X5, ctx->X6, ctx->X7, ctx->X8, ctx->X9);
    dbg_printf(" x10:%016lx x11:%016lx x12:%016lx x13:%016lx x14:%016lx\n",
               ctx->X10, ctx->X11, ctx->X12, ctx->X13, ctx->X14);
    dbg_printf(" x15:%016lx x16:%016lx x17:%016lx x18:%016lx x19:%016lx\n",
               ctx->X15, ctx->X16, ctx->X17, ctx->X18, ctx->X19);
    dbg_printf(" x20:%016lx x21:%016lx x22:%016lx x23:%016lx x24:%016lx\n",
               ctx->X20, ctx->X21, ctx->X22, ctx->X23, ctx->X24);
    dbg_printf(" x25:%016lx x26:%016lx x27:%016lx x28:%016lx x29:%016lx\n",
               ctx->X25, ctx->X26, ctx->X27, ctx->X28, ctx->X29);
    dbg_printf(" x30:%016lx\n", ctx->X30);

    if (all_regs) dbg_printf( "Floating point ARM64 dump not implemented\n" );
}

static void be_arm64_print_segment_info(HANDLE hThread, const CONTEXT* ctx)
{
}

static struct dbg_internal_var be_arm64_ctx[] =
{
    {CV_ARM64_X0 +  0,    "x0",     (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X0),     dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  1,    "x1",     (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X1),     dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  2,    "x2",     (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X2),     dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  3,    "x3",     (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X3),     dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  4,    "x4",     (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X4),     dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  5,    "x5",     (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X5),     dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  6,    "x6",     (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X6),     dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  7,    "x7",     (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X7),     dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  8,    "x8",     (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X8),     dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  9,    "x9",     (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X9),     dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  10,   "x10",    (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X10),    dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  11,   "x11",    (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X11),    dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  12,   "x12",    (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X12),    dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  13,   "x13",    (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X13),    dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  14,   "x14",    (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X14),    dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  15,   "x15",    (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X15),    dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  16,   "x16",    (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X16),    dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  17,   "x17",    (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X17),    dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  18,   "x18",    (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X18),    dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  19,   "x19",    (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X19),    dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  20,   "x20",    (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X20),    dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  21,   "x21",    (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X21),    dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  22,   "x22",    (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X22),    dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  23,   "x23",    (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X23),    dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  24,   "x24",    (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X24),    dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  25,   "x25",    (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X25),    dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  26,   "x26",    (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X26),    dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  27,   "x27",    (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X27),    dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  28,   "x28",    (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X28),    dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  29,   "x29",    (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X29),    dbg_itype_unsigned_long_int},
    {CV_ARM64_X0 +  30,   "x30",    (DWORD_PTR*)FIELD_OFFSET(CONTEXT, X30),    dbg_itype_unsigned_long_int},
    {CV_ARM64_SP,         "sp",     (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Sp),     dbg_itype_unsigned_long_int},
    {CV_ARM64_PC,         "pc",     (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Pc),     dbg_itype_unsigned_long_int},
    {CV_ARM64_PSTATE,     "pstate", (DWORD_PTR*)FIELD_OFFSET(CONTEXT, PState), dbg_itype_unsigned_long_int},
    {0,                   NULL,     0,                                         dbg_itype_none}
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
                                   CONTEXT* ctx, enum be_xpoint_type type,
                                   void* addr, unsigned long* val, unsigned size)
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
                                   CONTEXT* ctx, enum be_xpoint_type type,
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

static BOOL be_arm64_is_watchpoint_set(const CONTEXT* ctx, unsigned idx)
{
    dbg_printf("be_arm64_is_watchpoint_set: not done\n");
    return FALSE;
}

static void be_arm64_clear_watchpoint(CONTEXT* ctx, unsigned idx)
{
    dbg_printf("be_arm64_clear_watchpoint: not done\n");
}

static int be_arm64_adjust_pc_for_break(CONTEXT* ctx, BOOL way)
{
    if (way)
    {
        ctx->Pc -= 4;
        return -4;
    }
    ctx->Pc += 4;
    return 4;
}

static BOOL be_arm64_fetch_integer(const struct dbg_lvalue* lvalue, unsigned size,
                                   BOOL is_signed, LONGLONG* ret)
{
    if (size != 1 && size != 2 && size != 4 && size != 8) return FALSE;

    memset(ret, 0, sizeof(*ret)); /* clear unread bytes */
    /* FIXME: this assumes that debuggee and debugger use the same
     * integral representation
     */
    if (!memory_read_value(lvalue, size, ret)) return FALSE;

    /* propagate sign information */
    if (is_signed && size < 8 && (*ret >> (size * 8 - 1)) != 0)
    {
        ULONGLONG neg = -1;
        *ret |= neg << (size * 8);
    }
    return TRUE;
}

static BOOL be_arm64_fetch_float(const struct dbg_lvalue* lvalue, unsigned size,
                                 long double* ret)
{
    char tmp[sizeof(long double)];

    /* FIXME: this assumes that debuggee and debugger use the same
     * representation for reals
     */
    if (!memory_read_value(lvalue, size, tmp)) return FALSE;

    if (size == sizeof(float)) *ret = *(float*)tmp;
    else if (size == sizeof(double)) *ret = *(double*)tmp;
    else if (size == sizeof(long double)) *ret = *(long double*)tmp;
    else return FALSE;

    return TRUE;
}

static BOOL be_arm64_store_integer(const struct dbg_lvalue* lvalue, unsigned size,
                                   BOOL is_signed, LONGLONG val)
{
    /* this is simple if we're on a little endian CPU */
    return memory_write_value(lvalue, size, &val);
}

void be_arm64_disasm_one_insn(ADDRESS64 *addr, int display)
{
    dbg_printf("be_arm64_disasm_one_insn: not done\n");
}

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
    be_arm64_fetch_integer,
    be_arm64_fetch_float,
    be_arm64_store_integer,
};
#endif
