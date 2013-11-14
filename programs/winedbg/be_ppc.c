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

static BOOL be_ppc_get_addr(HANDLE hThread, const CONTEXT* ctx,
                            enum be_cpu_addr bca, ADDRESS64* addr)
{
    switch (bca)
    {
    case be_cpu_addr_pc:
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->Iar);
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

static void be_ppc_single_step(CONTEXT* ctx, BOOL enable)
{
#ifndef MSR_SE
# define MSR_SE (1<<10)
#endif 
    if (enable) ctx->Msr |= MSR_SE;
    else ctx->Msr &= ~MSR_SE;
}

static void be_ppc_print_context(HANDLE hThread, const CONTEXT* ctx, int all_regs)
{
    dbg_printf("Context printing for PPC not done yet\n");
}

static void be_ppc_print_segment_info(HANDLE hThread, const CONTEXT* ctx)
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
                                 CONTEXT* ctx, enum be_xpoint_type type,
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

static BOOL be_ppc_is_watchpoint_set(const CONTEXT* ctx, unsigned idx)
{
    dbg_printf("not done\n");
    return FALSE;
}

static void be_ppc_clear_watchpoint(CONTEXT* ctx, unsigned idx)
{
    dbg_printf("not done\n");
}

static int be_ppc_adjust_pc_for_break(CONTEXT* ctx, BOOL way)
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
};
#endif
