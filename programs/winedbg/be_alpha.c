/*
 * Debugger Alpha specific functions
 *
 * Copyright 2004 Vincent Béron
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

#if defined(__ALPHA__)

static unsigned be_alpha_get_addr(HANDLE hThread, const CONTEXT* ctx, 
                                 enum be_cpu_addr bca, ADDRESS64* addr)
{
    dbg_printf("not done\n");
    return FALSE;
}

static unsigned be_alpha_get_register_info(int regno, enum be_cpu_addr* kind)
{
    dbg_printf("not done\n");
    return FALSE;
}

static void be_alpha_single_step(CONTEXT* ctx, unsigned enable)
{
    dbg_printf("not done\n");
}

static void be_alpha_print_context(HANDLE hThread, const CONTEXT* ctx, int all_regs)
{
    dbg_printf("Context printing for Alpha not done yet\n");
}

static void be_alpha_print_segment_info(HANDLE hThread, const CONTEXT* ctx)
{
}

static struct dbg_internal_var be_alpha_ctx[] =
{
    {0,                 NULL,           0,                                      dbg_itype_none}
};

static unsigned be_alpha_is_step_over_insn(const void* insn)
{
    dbg_printf("not done\n");
    return FALSE;
}

static unsigned be_alpha_is_function_return(const void* insn)
{
    dbg_printf("not done\n");
    return FALSE;
}

static unsigned be_alpha_is_break_insn(const void* insn)
{
    dbg_printf("not done\n");
    return FALSE;
}

static unsigned be_alpha_is_func_call(const void* insn, ADDRESS64* callee)
{
    return FALSE;
}

static void be_alpha_disasm_one_insn(ADDRESS64* addr, int display)
{
    dbg_printf("Disasm NIY\n");
}

static unsigned be_alpha_insert_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                       CONTEXT* ctx, enum be_xpoint_type type,
                                       void* addr, unsigned long* val, unsigned size)
{
    unsigned long       xbp;
    SIZE_T              sz;

    switch (type)
    {
    case be_xpoint_break:
        if (!size) return 0;
        if (!pio->read(hProcess, addr, val, 4, &sz) || sz != 4) return 0;
        xbp = 0x7d821008; /* 7d 82 10 08 ... in big endian */
        if (!pio->write(hProcess, addr, &xbp, 4, &sz) || sz != 4) return 0;
        break;
    default:
        dbg_printf("Unknown/unsupported bp type %c\n", type);
        return 0;
    }
    return 1;
}

static unsigned be_alpha_remove_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                       CONTEXT* ctx, enum be_xpoint_type type, 
                                       void* addr, unsigned long val, unsigned size)
{
    dbg_printf("not done\n");
    return FALSE;
}

static unsigned be_alpha_is_watchpoint_set(const CONTEXT* ctx, unsigned idx)
{
    dbg_printf("not done\n");
    return FALSE;
}

static void be_alpha_clear_watchpoint(CONTEXT* ctx, unsigned idx)
{
    dbg_printf("not done\n");
}

static int be_alpha_adjust_pc_for_break(CONTEXT* ctx, BOOL way)
{
    dbg_printf("not done\n");
    return 0;
}

static int be_alpha_fetch_integer(const struct dbg_lvalue* lvalue, unsigned size,
                                  unsigned ext_sign, LONGLONG* ret)
{
    dbg_printf("not done\n");
    return FALSE;
}

static int be_alpha_fetch_float(const struct dbg_lvalue* lvalue, unsigned size, 
                                long double* ret)
{
    dbg_printf("not done\n");
    return FALSE;
}

struct backend_cpu be_alpha =
{
    IMAGE_FILE_MACHINE_ALPHA,
    8,
    be_cpu_linearize,
    be_cpu_build_addr,
    be_alpha_get_addr,
    be_alpha_get_register_info,
    be_alpha_single_step,
    be_alpha_print_context,
    be_alpha_print_segment_info,
    be_alpha_ctx,
    be_alpha_is_step_over_insn,
    be_alpha_is_function_return,
    be_alpha_is_break_insn,
    be_alpha_is_func_call,
    be_alpha_disasm_one_insn,
    be_alpha_insert_Xpoint,
    be_alpha_remove_Xpoint,
    be_alpha_is_watchpoint_set,
    be_alpha_clear_watchpoint,
    be_alpha_adjust_pc_for_break,
    be_alpha_fetch_integer,
    be_alpha_fetch_float,
};
#endif
