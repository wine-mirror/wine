/*
 * Debugger Sparc specific functions
 *
 * Copyright 2010 Austin English
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

#if defined(__sparc__)

static unsigned be_sparc_get_addr(HANDLE hThread, const CONTEXT* ctx,
                                 enum be_cpu_addr bca, ADDRESS64* addr)
{
    dbg_printf("not done for Sparc\n");
    return FALSE;
}

static unsigned be_sparc_get_register_info(int regno, enum be_cpu_addr* kind)
{
    dbg_printf("not done for Sparc\n");
    return FALSE;
}

static void be_sparc_single_step(CONTEXT* ctx, unsigned enable)
{
    dbg_printf("not done for Sparc\n");
}

static void be_sparc_print_context(HANDLE hThread, const CONTEXT* ctx, int all_regs)
{
    dbg_printf("not done for Sparc\n");
}

static void be_sparc_print_segment_info(HANDLE hThread, const CONTEXT* ctx)
{
    dbg_printf("not done for Sparc\n");
}

static struct dbg_internal_var be_sparc_ctx[] =
{
    {0, NULL, 0, dbg_itype_none}
};

static unsigned be_sparc_is_step_over_insn(const void* insn)
{
    dbg_printf("not done for Sparc\n");
    return FALSE;
}

static unsigned be_sparc_is_function_return(const void* insn)
{
    dbg_printf("not done for Sparc\n");
    return FALSE;
}

static unsigned be_sparc_is_break_insn(const void* insn)
{
    dbg_printf("not done for Sparc\n");
    return FALSE;
}

static unsigned be_sparc_is_func_call(const void* insn, ADDRESS64* callee)
{
    return FALSE;
}

static void be_sparc_disasm_one_insn(ADDRESS64* addr, int display)
{
    dbg_printf("not done for Sparc\n");
}

static unsigned be_sparc_insert_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                       CONTEXT* ctx, enum be_xpoint_type type,
                                       void* addr, unsigned long* val, unsigned size)
{
    dbg_printf("not done for Sparc\n");
    return 0;
}

static unsigned be_sparc_remove_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                       CONTEXT* ctx, enum be_xpoint_type type,
                                       void* addr, unsigned long val, unsigned size)
{
    dbg_printf("not done for Sparc\n");
    return FALSE;
}

static unsigned be_sparc_is_watchpoint_set(const CONTEXT* ctx, unsigned idx)
{
    dbg_printf("not done for Sparc\n");
    return FALSE;
}

static void be_sparc_clear_watchpoint(CONTEXT* ctx, unsigned idx)
{
    dbg_printf("not done for Sparc\n");
}

static int be_sparc_adjust_pc_for_break(CONTEXT* ctx, BOOL way)
{
    dbg_printf("not done for Sparc\n");
    return 0;
}

static int be_sparc_fetch_integer(const struct dbg_lvalue* lvalue, unsigned size,
                                  unsigned ext_sign, LONGLONG* ret)
{
    dbg_printf("not done for Sparc\n");
    return FALSE;
}

static int be_sparc_fetch_float(const struct dbg_lvalue* lvalue, unsigned size,
                                long double* ret)
{
    dbg_printf("not done for Sparc\n");
    return FALSE;
}

struct backend_cpu be_sparc =
{
    IMAGE_FILE_MACHINE_SPARC,
    4,
    be_cpu_linearize,
    be_cpu_build_addr,
    be_sparc_get_addr,
    be_sparc_get_register_info,
    be_sparc_single_step,
    be_sparc_print_context,
    be_sparc_print_segment_info,
    be_sparc_ctx,
    be_sparc_is_step_over_insn,
    be_sparc_is_function_return,
    be_sparc_is_break_insn,
    be_sparc_is_func_call,
    be_sparc_disasm_one_insn,
    be_sparc_insert_Xpoint,
    be_sparc_remove_Xpoint,
    be_sparc_is_watchpoint_set,
    be_sparc_clear_watchpoint,
    be_sparc_adjust_pc_for_break,
    be_sparc_fetch_integer,
    be_sparc_fetch_float,
};
#endif
