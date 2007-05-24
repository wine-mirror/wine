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
                                 enum be_cpu_addr bca, ADDRESS64* addr)
{
    addr->Mode = AddrModeFlat;
    switch (bca)
    {
    case be_cpu_addr_pc:
        addr->Segment = ctx->SegCs;
        addr->Offset = ctx->Rip;
        return TRUE;
    case be_cpu_addr_stack:
        addr->Segment = ctx->SegSs;
        addr->Offset = ctx->Rsp;
        return TRUE;
    case be_cpu_addr_frame:
        addr->Segment = ctx->SegSs;
        addr->Offset = ctx->Rbp;
        return TRUE;
    default:
        addr->Mode = -1;
        return FALSE;
    }
}

static unsigned be_x86_64_get_register_info(int regno, enum be_cpu_addr* kind)
{
    /* this is true when running in 32bit mode... and wrong in 64 :-/ */
    switch (regno)
    {
    case CV_AMD64_RIP: *kind = be_cpu_addr_pc; return TRUE;
    case CV_AMD64_EBP: *kind = be_cpu_addr_frame; return TRUE;
    case CV_AMD64_ESP: *kind = be_cpu_addr_stack; return TRUE;
    }
    return FALSE;
}

static void be_x86_64_single_step(CONTEXT* ctx, unsigned enable)
{
    dbg_printf("not done single_step\n");
}

static void be_x86_64_print_context(HANDLE hThread, const CONTEXT* ctx,
                                    int all_regs)
{
    dbg_printf("Context printing for x86_64 not done yet\n");
}

static void be_x86_64_print_segment_info(HANDLE hThread, const CONTEXT* ctx)
{
}

static struct dbg_internal_var be_x86_64_ctx[] =
{
    {CV_AMD64_AL,       "AL",           (DWORD*)FIELD_OFFSET(CONTEXT, Rax),     dbg_itype_unsigned_char_int},
    {CV_AMD64_BL,       "BL",           (DWORD*)FIELD_OFFSET(CONTEXT, Rbx),     dbg_itype_unsigned_char_int},
    {CV_AMD64_CL,       "CL",           (DWORD*)FIELD_OFFSET(CONTEXT, Rcx),     dbg_itype_unsigned_char_int},
    {CV_AMD64_DL,       "DL",           (DWORD*)FIELD_OFFSET(CONTEXT, Rdx),     dbg_itype_unsigned_char_int},
    {CV_AMD64_AH,       "AH",           (DWORD*)(FIELD_OFFSET(CONTEXT, Rax)+1), dbg_itype_unsigned_char_int},
    {CV_AMD64_BH,       "BH",           (DWORD*)(FIELD_OFFSET(CONTEXT, Rbx)+1), dbg_itype_unsigned_char_int},
    {CV_AMD64_CH,       "CH",           (DWORD*)(FIELD_OFFSET(CONTEXT, Rcx)+1), dbg_itype_unsigned_char_int},
    {CV_AMD64_DH,       "DH",           (DWORD*)(FIELD_OFFSET(CONTEXT, Rdx)+1), dbg_itype_unsigned_char_int},
    {CV_AMD64_AX,       "AX",           (DWORD*)FIELD_OFFSET(CONTEXT, Rax),     dbg_itype_unsigned_short_int},
    {CV_AMD64_BX,       "BX",           (DWORD*)FIELD_OFFSET(CONTEXT, Rbx),     dbg_itype_unsigned_short_int},
    {CV_AMD64_CX,       "CX",           (DWORD*)FIELD_OFFSET(CONTEXT, Rcx),     dbg_itype_unsigned_short_int},
    {CV_AMD64_DX,       "DX",           (DWORD*)FIELD_OFFSET(CONTEXT, Rdx),     dbg_itype_unsigned_short_int},
    {CV_AMD64_SP,       "SP",           (DWORD*)FIELD_OFFSET(CONTEXT, Rsp),     dbg_itype_unsigned_short_int},
    {CV_AMD64_BP,       "BP",           (DWORD*)FIELD_OFFSET(CONTEXT, Rbp),     dbg_itype_unsigned_short_int},
    {CV_AMD64_SI,       "SI",           (DWORD*)FIELD_OFFSET(CONTEXT, Rsi),     dbg_itype_unsigned_short_int},
    {CV_AMD64_DI,       "DI",           (DWORD*)FIELD_OFFSET(CONTEXT, Rdi),     dbg_itype_unsigned_short_int},
    {CV_AMD64_EAX,      "EAX",          (DWORD*)FIELD_OFFSET(CONTEXT, Rax),     dbg_itype_unsigned_int},
    {CV_AMD64_EBX,      "EBX",          (DWORD*)FIELD_OFFSET(CONTEXT, Rbx),     dbg_itype_unsigned_int},
    {CV_AMD64_ECX,      "ECX",          (DWORD*)FIELD_OFFSET(CONTEXT, Rcx),     dbg_itype_unsigned_int},
    {CV_AMD64_EDX,      "EDX",          (DWORD*)FIELD_OFFSET(CONTEXT, Rdx),     dbg_itype_unsigned_int},
    {CV_AMD64_ESP,      "ESP",          (DWORD*)FIELD_OFFSET(CONTEXT, Rsp),     dbg_itype_unsigned_int},
    {CV_AMD64_EBP,      "EBP",          (DWORD*)FIELD_OFFSET(CONTEXT, Rbp),     dbg_itype_unsigned_int},
    {CV_AMD64_ESI,      "ESI",          (DWORD*)FIELD_OFFSET(CONTEXT, Rsi),     dbg_itype_unsigned_int},
    {CV_AMD64_EDI,      "EDI",          (DWORD*)FIELD_OFFSET(CONTEXT, Rdi),     dbg_itype_unsigned_int},
    {CV_AMD64_ES,       "ES",           (DWORD*)FIELD_OFFSET(CONTEXT, SegEs),   dbg_itype_unsigned_short_int},
    {CV_AMD64_CS,       "CS",           (DWORD*)FIELD_OFFSET(CONTEXT, SegCs),   dbg_itype_unsigned_short_int},
    {CV_AMD64_SS,       "SS",           (DWORD*)FIELD_OFFSET(CONTEXT, SegSs),   dbg_itype_unsigned_short_int},
    {CV_AMD64_DS,       "DS",           (DWORD*)FIELD_OFFSET(CONTEXT, SegDs),   dbg_itype_unsigned_short_int},
    {CV_AMD64_FS,       "FS",           (DWORD*)FIELD_OFFSET(CONTEXT, SegFs),   dbg_itype_unsigned_short_int},
    {CV_AMD64_GS,       "GS",           (DWORD*)FIELD_OFFSET(CONTEXT, SegGs),   dbg_itype_unsigned_short_int},
    {CV_AMD64_FLAGS,    "FLAGS",        (DWORD*)FIELD_OFFSET(CONTEXT, EFlags),  dbg_itype_unsigned_short_int},
    {CV_AMD64_EFLAGS,   "EFLAGS",       (DWORD*)FIELD_OFFSET(CONTEXT, EFlags),  dbg_itype_unsigned_int},
    {CV_AMD64_RIP,      "RIP",          (DWORD*)FIELD_OFFSET(CONTEXT, Rip),     dbg_itype_unsigned_int},
    {CV_AMD64_RAX,      "RAX",          (DWORD*)FIELD_OFFSET(CONTEXT, Rax),     dbg_itype_unsigned_long_int},
    {CV_AMD64_RBX,      "RBX",          (DWORD*)FIELD_OFFSET(CONTEXT, Rbx),     dbg_itype_unsigned_long_int},
    {CV_AMD64_RCX,      "RCX",          (DWORD*)FIELD_OFFSET(CONTEXT, Rcx),     dbg_itype_unsigned_long_int},
    {CV_AMD64_RDX,      "RDX",          (DWORD*)FIELD_OFFSET(CONTEXT, Rdx),     dbg_itype_unsigned_long_int},
    {CV_AMD64_RSP,      "RSP",          (DWORD*)FIELD_OFFSET(CONTEXT, Rsp),     dbg_itype_unsigned_long_int},
    {CV_AMD64_RBP,      "RBP",          (DWORD*)FIELD_OFFSET(CONTEXT, Rbp),     dbg_itype_unsigned_long_int},
    {CV_AMD64_RSI,      "RSI",          (DWORD*)FIELD_OFFSET(CONTEXT, Rsi),     dbg_itype_unsigned_long_int},
    {CV_AMD64_RDI,      "RDI",          (DWORD*)FIELD_OFFSET(CONTEXT, Rdi),     dbg_itype_unsigned_long_int},
    {CV_AMD64_R8,       "R8",           (DWORD*)FIELD_OFFSET(CONTEXT, R8),      dbg_itype_unsigned_long_int},
    {CV_AMD64_R9,       "R9",           (DWORD*)FIELD_OFFSET(CONTEXT, R9),      dbg_itype_unsigned_long_int},
    {CV_AMD64_R10,      "R10",          (DWORD*)FIELD_OFFSET(CONTEXT, R10),     dbg_itype_unsigned_long_int},
    {CV_AMD64_R11,      "R11",          (DWORD*)FIELD_OFFSET(CONTEXT, R11),     dbg_itype_unsigned_long_int},
    {CV_AMD64_R12,      "R12",          (DWORD*)FIELD_OFFSET(CONTEXT, R12),     dbg_itype_unsigned_long_int},
    {CV_AMD64_R13,      "R13",          (DWORD*)FIELD_OFFSET(CONTEXT, R13),     dbg_itype_unsigned_long_int},
    {CV_AMD64_R14,      "R14",          (DWORD*)FIELD_OFFSET(CONTEXT, R14),     dbg_itype_unsigned_long_int},
    {CV_AMD64_R15,      "R15",          (DWORD*)FIELD_OFFSET(CONTEXT, R15),     dbg_itype_unsigned_long_int},
    {0,                 NULL,           0,                                      dbg_itype_none}
};

static const struct dbg_internal_var* be_x86_64_init_registers(CONTEXT* ctx)
{
    struct dbg_internal_var*    div;

    for (div = be_x86_64_ctx; div->name; div++)
        div->pval = (DWORD*)((char*)ctx + (DWORD_PTR)div->pval);
    return be_x86_64_ctx;
}

static unsigned be_x86_64_is_step_over_insn(const void* insn)
{
    dbg_printf("not done step_over_insn\n");
    return FALSE;
}

static unsigned be_x86_64_is_function_return(const void* insn)
{
    dbg_printf("not done is_function_return\n");
    return FALSE;
}

static unsigned be_x86_64_is_break_insn(const void* insn)
{
    dbg_printf("not done is_break_insn\n");
    return FALSE;
}

static unsigned be_x86_64_is_func_call(const void* insn, ADDRESS64* callee)
{
    dbg_printf("not done is_func_call\n");
    return FALSE;
}

static void be_x86_64_disasm_one_insn(ADDRESS64* addr, int display)
{
    dbg_printf("Disasm NIY\n");
}

static unsigned be_x86_64_insert_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                       CONTEXT* ctx, enum be_xpoint_type type,
                                       void* addr, unsigned long* val, unsigned size)
{
    dbg_printf("not done insert_Xpoint\n");
    return 0;
}

static unsigned be_x86_64_remove_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                       CONTEXT* ctx, enum be_xpoint_type type, 
                                       void* addr, unsigned long val, unsigned size)
{
    dbg_printf("not done remove_Xpoint\n");
    return FALSE;
}

static unsigned be_x86_64_is_watchpoint_set(const CONTEXT* ctx, unsigned idx)
{
    dbg_printf("not done is_watchpoint_set\n");
    return FALSE;
}

static void be_x86_64_clear_watchpoint(CONTEXT* ctx, unsigned idx)
{
    dbg_printf("not done clear_watchpoint\n");
}

static int be_x86_64_adjust_pc_for_break(CONTEXT* ctx, BOOL way)
{
    if (way)
    {
        ctx->Rip--;
        return -1;
    }
    ctx->Rip++;
    return 1;
}

static int be_x86_64_fetch_integer(const struct dbg_lvalue* lvalue, unsigned size,
                                  unsigned ext_sign, LONGLONG* ret)
{
    dbg_printf("not done fetch_integer\n");
    return FALSE;
}

static int be_x86_64_fetch_float(const struct dbg_lvalue* lvalue, unsigned size, 
                                long double* ret)
{
    dbg_printf("not done fetch_float\n");
    return FALSE;
}

struct backend_cpu be_x86_64 =
{
    be_cpu_linearize,
    be_cpu_build_addr,
    be_x86_64_get_addr,
    be_x86_64_get_register_info,
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
