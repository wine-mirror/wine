/*
 * Debugger x86_64 specific functions
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

#if defined(__x86_64__)

static unsigned be_x86_64_get_addr(HANDLE hThread, const CONTEXT* ctx, 
                                 enum be_cpu_addr bca, ADDRESS* addr)
{
    dbg_printf("not done\n");
    return FALSE;
}

static void be_x86_64_single_step(CONTEXT* ctx, unsigned enable)
{
    dbg_printf("not done\n");
}

static void be_x86_64_print_context(HANDLE hThread, const CONTEXT* ctx)
{
    dbg_printf("Context printing for x86_64 not done yet\n");
}

static void be_x86_64_print_segment_info(HANDLE hThread, const CONTEXT* ctx)
{
}

static struct dbg_internal_var be_x86_64_ctx[] =
{
    {0,                 NULL,           0,                                      dbg_itype_none}
};

static const struct dbg_internal_var* be_x86_64_init_registers(CONTEXT* ctx)
{
    dbg_printf("not done\n");
    return be_x86_64_ctx;
}

static unsigned be_x86_64_is_step_over_insn(void* insn)
{
    dbg_printf("not done\n");
    return FALSE;
}

static unsigned be_x86_64_is_function_return(void* insn)
{
    dbg_printf("not done\n");
    return FALSE;
}

static unsigned be_x86_64_is_break_insn(void* insn)
{
    dbg_printf("not done\n");
    return FALSE;
}

static unsigned be_x86_64_is_func_call(void* insn, void** insn_callee)
{
    return FALSE;
}

static void be_x86_64_disasm_one_insn(ADDRESS* addr, int display)
{
    dbg_printf("Disasm NIY\n");
}

static unsigned be_x86_64_insert_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                       CONTEXT* ctx, enum be_xpoint_type type,
                                       void* addr, unsigned long* val, unsigned size)
{
    dbg_printf("not done\n");
    return 0;
}

static unsigned be_x86_64_remove_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                       CONTEXT* ctx, enum be_xpoint_type type, 
                                       void* addr, unsigned long val, unsigned size)
{
    dbg_printf("not done\n");
    return FALSE;
}

static unsigned be_x86_64_is_watchpoint_set(const CONTEXT* ctx, unsigned idx)
{
    dbg_printf("not done\n");
    return FALSE;
}

static void be_x86_64_clear_watchpoint(CONTEXT* ctx, unsigned idx)
{
    dbg_printf("not done\n");
}

static int be_x86_64_adjust_pc_for_break(CONTEXT* ctx, BOOL way)
{
    dbg_printf("not done\n");
    return 0;
}

static int be_x86_64_fetch_integer(const struct dbg_lvalue* lvalue, unsigned size,
                                  unsigned ext_sign, LONGLONG* ret)
{
    dbg_printf("not done\n");
    return FALSE;
}

static int be_x86_64_fetch_float(const struct dbg_lvalue* lvalue, unsigned size, 
                                long double* ret)
{
    dbg_printf("not done\n");
    return FALSE;
}

struct backend_cpu be_x86_64 =
{
    be_cpu_linearize,
    be_cpu_build_addr,
    be_x86_64_get_addr,
    be_x86_64_single_step,
    be_x86_64_print_context,
    be_x86_64_print_segment_info,
    be_x86_64_init_registers,
    be_x86_64_is_step_over_insn,
    be_x86_64_is_function_return,
    be_x86_64_is_break_insn,
    be_x86_64_is_func_call,
    be_x86_64_disasm_one_insn,
    be_x86_64_insert_Xpoint,
    be_x86_64_remove_Xpoint,
    be_x86_64_is_watchpoint_set,
    be_x86_64_clear_watchpoint,
    be_x86_64_adjust_pc_for_break,
    be_x86_64_fetch_integer,
    be_x86_64_fetch_float,
};
#endif
