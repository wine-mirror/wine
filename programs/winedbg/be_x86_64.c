/*
 * Debugger x86_64 specific functions
 *
 * Copyright 2004 Vincent Béron
 * Copyright 2009 Eric Pouech
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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winedbg);

#if defined(__x86_64__)

#define STEP_FLAG 0x00000100 /* single step flag */

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
    if (enable) ctx->EFlags |= STEP_FLAG;
    else ctx->EFlags &= ~STEP_FLAG;
}

static void be_x86_64_print_context(HANDLE hThread, const CONTEXT* ctx,
                                    int all_regs)
{
    static const char flags[] = "aVR-N--ODITSZ-A-P-C";
    char buf[33];
    int i;

    strcpy(buf, flags);
    for (i = 0; buf[i]; i++)
        if (buf[i] != '-' && !(ctx->EFlags & (1 << (sizeof(flags) - 2 - i))))
            buf[i] = ' ';

    dbg_printf("Register dump:\n");
    dbg_printf(" rip:%016lx rsp:%016lx rbp:%016lx eflags:%08x (%s)\n",
               ctx->Rip, ctx->Rsp, ctx->Rbp, ctx->EFlags, buf);
    dbg_printf(" rax:%016lx rbx:%016lx rcx:%016lx rdx:%016lx\n",
               ctx->Rax, ctx->Rbx, ctx->Rcx, ctx->Rdx);
    dbg_printf(" rsi:%016lx rdi:%016lx  r8:%016lx  r9:%016lx r10:%016lx\n",
               ctx->Rsi, ctx->Rdi, ctx->R8, ctx->R9, ctx->R10 );
    dbg_printf(" r11:%016lx r12:%016lx r13:%016lx r14:%016lx r15:%016lx\n",
               ctx->R11, ctx->R12, ctx->R13, ctx->R14, ctx->R15 );

    if (all_regs) dbg_printf( "Floating point x86_64 dump not implemented\n" );
}

static void be_x86_64_print_segment_info(HANDLE hThread, const CONTEXT* ctx)
{
}

static struct dbg_internal_var be_x86_64_ctx[] =
{
    {CV_AMD64_AL,       "AL",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rax),     dbg_itype_unsigned_char_int},
    {CV_AMD64_BL,       "BL",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rbx),     dbg_itype_unsigned_char_int},
    {CV_AMD64_CL,       "CL",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rcx),     dbg_itype_unsigned_char_int},
    {CV_AMD64_DL,       "DL",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rdx),     dbg_itype_unsigned_char_int},
    {CV_AMD64_AH,       "AH",           (DWORD_PTR*)(FIELD_OFFSET(CONTEXT, Rax)+1), dbg_itype_unsigned_char_int},
    {CV_AMD64_BH,       "BH",           (DWORD_PTR*)(FIELD_OFFSET(CONTEXT, Rbx)+1), dbg_itype_unsigned_char_int},
    {CV_AMD64_CH,       "CH",           (DWORD_PTR*)(FIELD_OFFSET(CONTEXT, Rcx)+1), dbg_itype_unsigned_char_int},
    {CV_AMD64_DH,       "DH",           (DWORD_PTR*)(FIELD_OFFSET(CONTEXT, Rdx)+1), dbg_itype_unsigned_char_int},
    {CV_AMD64_AX,       "AX",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rax),     dbg_itype_unsigned_short_int},
    {CV_AMD64_BX,       "BX",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rbx),     dbg_itype_unsigned_short_int},
    {CV_AMD64_CX,       "CX",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rcx),     dbg_itype_unsigned_short_int},
    {CV_AMD64_DX,       "DX",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rdx),     dbg_itype_unsigned_short_int},
    {CV_AMD64_SP,       "SP",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rsp),     dbg_itype_unsigned_short_int},
    {CV_AMD64_BP,       "BP",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rbp),     dbg_itype_unsigned_short_int},
    {CV_AMD64_SI,       "SI",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rsi),     dbg_itype_unsigned_short_int},
    {CV_AMD64_DI,       "DI",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rdi),     dbg_itype_unsigned_short_int},
    {CV_AMD64_EAX,      "EAX",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rax),     dbg_itype_unsigned_int},
    {CV_AMD64_EBX,      "EBX",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rbx),     dbg_itype_unsigned_int},
    {CV_AMD64_ECX,      "ECX",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rcx),     dbg_itype_unsigned_int},
    {CV_AMD64_EDX,      "EDX",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rdx),     dbg_itype_unsigned_int},
    {CV_AMD64_ESP,      "ESP",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rsp),     dbg_itype_unsigned_int},
    {CV_AMD64_EBP,      "EBP",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rbp),     dbg_itype_unsigned_int},
    {CV_AMD64_ESI,      "ESI",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rsi),     dbg_itype_unsigned_int},
    {CV_AMD64_EDI,      "EDI",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rdi),     dbg_itype_unsigned_int},
    {CV_AMD64_ES,       "ES",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, SegEs),   dbg_itype_unsigned_short_int},
    {CV_AMD64_CS,       "CS",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, SegCs),   dbg_itype_unsigned_short_int},
    {CV_AMD64_SS,       "SS",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, SegSs),   dbg_itype_unsigned_short_int},
    {CV_AMD64_DS,       "DS",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, SegDs),   dbg_itype_unsigned_short_int},
    {CV_AMD64_FS,       "FS",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, SegFs),   dbg_itype_unsigned_short_int},
    {CV_AMD64_GS,       "GS",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, SegGs),   dbg_itype_unsigned_short_int},
    {CV_AMD64_FLAGS,    "FLAGS",        (DWORD_PTR*)FIELD_OFFSET(CONTEXT, EFlags),  dbg_itype_unsigned_short_int},
    {CV_AMD64_EFLAGS,   "EFLAGS",       (DWORD_PTR*)FIELD_OFFSET(CONTEXT, EFlags),  dbg_itype_unsigned_int},
    {CV_AMD64_RIP,      "RIP",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rip),     dbg_itype_unsigned_int},
    {CV_AMD64_RAX,      "RAX",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rax),     dbg_itype_unsigned_long_int},
    {CV_AMD64_RBX,      "RBX",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rbx),     dbg_itype_unsigned_long_int},
    {CV_AMD64_RCX,      "RCX",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rcx),     dbg_itype_unsigned_long_int},
    {CV_AMD64_RDX,      "RDX",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rdx),     dbg_itype_unsigned_long_int},
    {CV_AMD64_RSP,      "RSP",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rsp),     dbg_itype_unsigned_long_int},
    {CV_AMD64_RBP,      "RBP",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rbp),     dbg_itype_unsigned_long_int},
    {CV_AMD64_RSI,      "RSI",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rsi),     dbg_itype_unsigned_long_int},
    {CV_AMD64_RDI,      "RDI",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rdi),     dbg_itype_unsigned_long_int},
    {CV_AMD64_R8,       "R8",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R8),      dbg_itype_unsigned_long_int},
    {CV_AMD64_R9,       "R9",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R9),      dbg_itype_unsigned_long_int},
    {CV_AMD64_R10,      "R10",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R10),     dbg_itype_unsigned_long_int},
    {CV_AMD64_R11,      "R11",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R11),     dbg_itype_unsigned_long_int},
    {CV_AMD64_R12,      "R12",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R12),     dbg_itype_unsigned_long_int},
    {CV_AMD64_R13,      "R13",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R13),     dbg_itype_unsigned_long_int},
    {CV_AMD64_R14,      "R14",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R14),     dbg_itype_unsigned_long_int},
    {CV_AMD64_R15,      "R15",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R15),     dbg_itype_unsigned_long_int},
    {0,                 NULL,           0,                                      dbg_itype_none}
};

static const struct dbg_internal_var* be_x86_64_init_registers(CONTEXT* ctx)
{
    struct dbg_internal_var*    div;

    for (div = be_x86_64_ctx; div->name; div++)
        div->pval = (DWORD_PTR*)((char*)ctx + (DWORD_PTR)div->pval);
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

#define DR7_CONTROL_SHIFT	16
#define DR7_CONTROL_SIZE 	4

#define DR7_RW_EXECUTE 		(0x0)
#define DR7_RW_WRITE		(0x1)
#define DR7_RW_READ		(0x3)

#define DR7_LEN_1		(0x0)
#define DR7_LEN_2		(0x4)
#define DR7_LEN_4		(0xC)

#define DR7_LOCAL_ENABLE_SHIFT	0
#define DR7_GLOBAL_ENABLE_SHIFT 1
#define DR7_ENABLE_SIZE 	2

#define DR7_LOCAL_ENABLE_MASK	(0x55)
#define DR7_GLOBAL_ENABLE_MASK	(0xAA)

#define DR7_CONTROL_RESERVED	(0xFC00)
#define DR7_LOCAL_SLOWDOWN	(0x100)
#define DR7_GLOBAL_SLOWDOWN	(0x200)

#define	DR7_ENABLE_MASK(dr)	(1<<(DR7_LOCAL_ENABLE_SHIFT+DR7_ENABLE_SIZE*(dr)))
#define	IS_DR7_SET(ctrl,dr) 	((ctrl)&DR7_ENABLE_MASK(dr))

static inline int be_x86_64_get_unused_DR(CONTEXT* ctx, DWORD64** r)
{
    if (!IS_DR7_SET(ctx->Dr7, 0))
    {
        *r = &ctx->Dr0;
        return 0;
    }
    if (!IS_DR7_SET(ctx->Dr7, 1))
    {
        *r = &ctx->Dr1;
        return 1;
    }
    if (!IS_DR7_SET(ctx->Dr7, 2))
    {
        *r = &ctx->Dr2;
        return 2;
    }
    if (!IS_DR7_SET(ctx->Dr7, 3))
    {
        *r = &ctx->Dr3;
        return 3;
    }
    dbg_printf("All hardware registers have been used\n");

    return -1;
}

static unsigned be_x86_64_insert_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                       CONTEXT* ctx, enum be_xpoint_type type,
                                       void* addr, unsigned long* val, unsigned size)
{
    unsigned char       ch;
    SIZE_T              sz;
    DWORD64            *pr;
    int                 reg;
    unsigned long       bits;

    switch (type)
    {
    case be_xpoint_break:
        if (size != 0) return 0;
        if (!pio->read(hProcess, addr, &ch, 1, &sz) || sz != 1) return 0;
        *val = ch;
        ch = 0xcc;
        if (!pio->write(hProcess, addr, &ch, 1, &sz) || sz != 1) return 0;
        break;
    case be_xpoint_watch_exec:
        bits = DR7_RW_EXECUTE;
        goto hw_bp;
    case be_xpoint_watch_read:
        bits = DR7_RW_READ;
        goto hw_bp;
    case be_xpoint_watch_write:
        bits = DR7_RW_WRITE;
    hw_bp:
        if ((reg = be_x86_64_get_unused_DR(ctx, &pr)) == -1) return 0;
        *pr = (DWORD64)addr;
        if (type != be_xpoint_watch_exec) switch (size)
        {
        case 4: bits |= DR7_LEN_4; break;
        case 2: bits |= DR7_LEN_2; break;
        case 1: bits |= DR7_LEN_1; break;
        default: return 0;
        }
        *val = reg;
        /* clear old values */
        ctx->Dr7 &= ~(0x0F << (DR7_CONTROL_SHIFT + DR7_CONTROL_SIZE * reg));
        /* set the correct ones */
        ctx->Dr7 |= bits << (DR7_CONTROL_SHIFT + DR7_CONTROL_SIZE * reg);
	ctx->Dr7 |= DR7_ENABLE_MASK(reg) | DR7_LOCAL_SLOWDOWN;
        break;
    default:
        dbg_printf("Unknown bp type %c\n", type);
        return 0;
    }
    return 1;
}

static unsigned be_x86_64_remove_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                       CONTEXT* ctx, enum be_xpoint_type type, 
                                       void* addr, unsigned long val, unsigned size)
{
    SIZE_T              sz;
    unsigned char       ch;

    switch (type)
    {
    case be_xpoint_break:
        if (size != 0) return 0;
        if (!pio->read(hProcess, addr, &ch, 1, &sz) || sz != 1) return 0;
        if (ch != (unsigned char)0xCC)
            WINE_FIXME("Cannot get back %02x instead of 0xCC at %08lx\n",
                       ch, (unsigned long)addr);
        ch = (unsigned char)val;
        if (!pio->write(hProcess, addr, &ch, 1, &sz) || sz != 1) return 0;
        break;
    case be_xpoint_watch_exec:
    case be_xpoint_watch_read:
    case be_xpoint_watch_write:
        /* simply disable the entry */
        ctx->Dr7 &= ~DR7_ENABLE_MASK(val);
        break;
    default:
        dbg_printf("Unknown bp type %c\n", type);
        return 0;
    }
    return 1;
}

static unsigned be_x86_64_is_watchpoint_set(const CONTEXT* ctx, unsigned idx)
{
    return ctx->Dr6 & (1 << idx);
}

static void be_x86_64_clear_watchpoint(CONTEXT* ctx, unsigned idx)
{
    ctx->Dr6 &= ~(1 << idx);
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
    if (size != 1 && size != 2 && size != 4 && size != 8 && size != 16)
        return FALSE;

    memset(ret, 0, sizeof(*ret)); /* clear unread bytes */
    /* FIXME: this assumes that debuggee and debugger use the same
     * integral representation
     */
    if (!memory_read_value(lvalue, size, ret)) return FALSE;

    /* propagate sign information */
    if (ext_sign && size < 16 && (*ret >> (size * 8 - 1)) != 0)
    {
        ULONGLONG neg = -1;
        *ret |= neg << (size * 8);
    }
    return TRUE;
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
