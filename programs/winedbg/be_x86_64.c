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

static BOOL be_x86_64_get_addr(HANDLE hThread, const CONTEXT* ctx,
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

static BOOL be_x86_64_get_register_info(int regno, enum be_cpu_addr* kind)
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

static void be_x86_64_single_step(CONTEXT* ctx, BOOL enable)
{
    if (enable) ctx->EFlags |= STEP_FLAG;
    else ctx->EFlags &= ~STEP_FLAG;
}

static inline long double m128a_to_longdouble(const M128A m)
{
    /* gcc uses the same IEEE-754 representation as M128A for long double
     * but 16 byte aligned (hence only the first 10 bytes out of the 16 are used)
     */
    return *(long double*)&m;
}

static void be_x86_64_print_context(HANDLE hThread, const CONTEXT* ctx,
                                    int all_regs)
{
    static const char mxcsr_flags[16][4] = { "IE", "DE", "ZE", "OE", "UE", "PE", "DAZ", "IM",
                                             "DM", "ZM", "OM", "UM", "PM", "R-", "R+", "FZ" };
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

    if (!all_regs) return;

    dbg_printf("  cs:%04x  ds:%04x  es:%04x  fs:%04x  gs:%04x  ss:%04x\n",
               ctx->SegCs, ctx->SegDs, ctx->SegEs, ctx->SegFs, ctx->SegGs, ctx->SegSs );

    dbg_printf("Debug:\n");
    dbg_printf(" dr0:%016lx dr1:%016lx dr2:%016lx dr3:%016lx\n",
               ctx->Dr0, ctx->Dr1, ctx->Dr2, ctx->Dr3 );
    dbg_printf(" dr6:%016lx dr7:%016lx\n", ctx->Dr6, ctx->Dr7 );

    dbg_printf("Floating point:\n");
    dbg_printf(" flcw:%04x ", LOWORD(ctx->u.FltSave.ControlWord));
    dbg_printf(" fltw:%04x ", LOWORD(ctx->u.FltSave.TagWord));
    dbg_printf(" flsw:%04x", LOWORD(ctx->u.FltSave.StatusWord));

    dbg_printf("(cc:%d%d%d%d", (ctx->u.FltSave.StatusWord & 0x00004000) >> 14,
               (ctx->u.FltSave.StatusWord & 0x00000400) >> 10,
               (ctx->u.FltSave.StatusWord & 0x00000200) >> 9,
               (ctx->u.FltSave.StatusWord & 0x00000100) >> 8);

    dbg_printf(" top:%01x", (unsigned int) (ctx->u.FltSave.StatusWord & 0x00003800) >> 11);

    if (ctx->u.FltSave.StatusWord & 0x00000001)     /* Invalid Fl OP */
    {
       if (ctx->u.FltSave.StatusWord & 0x00000040)  /* Stack Fault */
       {
          if (ctx->u.FltSave.StatusWord & 0x00000200) /* C1 says Overflow */
             dbg_printf(" #IE(Stack Overflow)");
          else
             dbg_printf(" #IE(Stack Underflow)");     /* Underflow */
       }
       else  dbg_printf(" #IE(Arithmetic error)");    /* Invalid Fl OP */
    }
    if (ctx->u.FltSave.StatusWord & 0x00000002) dbg_printf(" #DE"); /* Denormalised OP */
    if (ctx->u.FltSave.StatusWord & 0x00000004) dbg_printf(" #ZE"); /* Zero Divide */
    if (ctx->u.FltSave.StatusWord & 0x00000008) dbg_printf(" #OE"); /* Overflow */
    if (ctx->u.FltSave.StatusWord & 0x00000010) dbg_printf(" #UE"); /* Underflow */
    if (ctx->u.FltSave.StatusWord & 0x00000020) dbg_printf(" #PE"); /* Precision error */
    if (ctx->u.FltSave.StatusWord & 0x00000040)
       if (!(ctx->u.FltSave.StatusWord & 0x00000001))
           dbg_printf(" #SE");                 /* Stack Fault (don't think this can occur) */
    if (ctx->u.FltSave.StatusWord & 0x00000080) dbg_printf(" #ES"); /* Error Summary */
    if (ctx->u.FltSave.StatusWord & 0x00008000) dbg_printf(" #FB"); /* FPU Busy */
    dbg_printf(")\n");
    dbg_printf(" flerr:%04x:%08x   fldata:%04x:%08x\n",
               ctx->u.FltSave.ErrorSelector, ctx->u.FltSave.ErrorOffset,
               ctx->u.FltSave.DataSelector, ctx->u.FltSave.DataOffset );

    for (i = 0; i < 4; i++)
    {
        dbg_printf(" st%u:%-16Lg ", i, m128a_to_longdouble(ctx->u.FltSave.FloatRegisters[i]));
    }
    dbg_printf("\n");
    for (i = 4; i < 8; i++)
    {
        dbg_printf(" st%u:%-16Lg ", i, m128a_to_longdouble(ctx->u.FltSave.FloatRegisters[i]));
    }
    dbg_printf("\n");

    dbg_printf(" mxcsr: %04x (", ctx->u.FltSave.MxCsr );
    for (i = 0; i < 16; i++)
        if (ctx->u.FltSave.MxCsr & (1 << i)) dbg_printf( " %s", mxcsr_flags[i] );
    dbg_printf(" )\n");

    for (i = 0; i < 16; i++)
    {
        dbg_printf( " %sxmm%u: uint=%016lx%016lx", (i > 9) ? "" : " ", i,
                    ctx->u.FltSave.XmmRegisters[i].High, ctx->u.FltSave.XmmRegisters[i].Low );
        dbg_printf( " double={%g; %g}", *(double *)&ctx->u.FltSave.XmmRegisters[i].Low,
                    *(double *)&ctx->u.FltSave.XmmRegisters[i].High );
        dbg_printf( " float={%g; %g; %g; %g}\n",
                    (double)*((float *)&ctx->u.FltSave.XmmRegisters[i] + 0),
                    (double)*((float *)&ctx->u.FltSave.XmmRegisters[i] + 1),
                    (double)*((float *)&ctx->u.FltSave.XmmRegisters[i] + 2),
                    (double)*((float *)&ctx->u.FltSave.XmmRegisters[i] + 3) );
    }
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
    {CV_AMD64_RIP,      "RIP",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Rip),     dbg_itype_unsigned_long_int},
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
    {0,                 NULL,           0,                                          dbg_itype_none}
};

#define	f_mod(b)	((b)>>6)
#define	f_reg(b)	(((b)>>3)&0x7)
#define	f_rm(b)		((b)&0x7)

static BOOL be_x86_64_is_step_over_insn(const void* insn)
{
    BYTE	ch;

    for (;;)
    {
        if (!dbg_read_memory(insn, &ch, sizeof(ch))) return FALSE;

        switch (ch)
        {
        /* Skip all prefixes */
        case 0x2e:  /* cs: */
        case 0x36:  /* ss: */
        case 0x3e:  /* ds: */
        case 0x26:  /* es: */
        case 0x64:  /* fs: */
        case 0x65:  /* gs: */
        case 0x66:  /* opcode size prefix */
        case 0x67:  /* addr size prefix */
        case 0xf0:  /* lock */
        case 0xf2:  /* repne */
        case 0xf3:  /* repe */
            insn = (const char*)insn + 1;
            continue;

        /* Handle call instructions */
        case 0xcd:  /* int <intno> */
        case 0xe8:  /* call <offset> */
        case 0x9a:  /* lcall <seg>:<off> */
            return TRUE;

        case 0xff:  /* call <regmodrm> */
	    if (!dbg_read_memory((const char*)insn + 1, &ch, sizeof(ch)))
                return FALSE;
	    return (((ch & 0x38) == 0x10) || ((ch & 0x38) == 0x18));

        /* Handle string instructions */
        case 0x6c:  /* insb */
        case 0x6d:  /* insw */
        case 0x6e:  /* outsb */
        case 0x6f:  /* outsw */
        case 0xa4:  /* movsb */
        case 0xa5:  /* movsw */
        case 0xa6:  /* cmpsb */
        case 0xa7:  /* cmpsw */
        case 0xaa:  /* stosb */
        case 0xab:  /* stosw */
        case 0xac:  /* lodsb */
        case 0xad:  /* lodsw */
        case 0xae:  /* scasb */
        case 0xaf:  /* scasw */
            return TRUE;

        default:
            return FALSE;
        }
    }
}

static BOOL be_x86_64_is_function_return(const void* insn)
{
    BYTE c;

    /* sigh... amd64 for prefetch optimization requires 'rep ret' in some cases */
    if (!dbg_read_memory(insn, &c, sizeof(c))) return FALSE;
    if (c == 0xF3) /* REP */
    {
        insn = (const char*)insn + 1;
        if (!dbg_read_memory(insn, &c, sizeof(c))) return FALSE;
    }
    return c == 0xC2 /* ret */ || c == 0xC3 /* ret NN */;
}

static BOOL be_x86_64_is_break_insn(const void* insn)
{
    BYTE        c;
    return dbg_read_memory(insn, &c, sizeof(c)) && c == 0xCC;
}

static BOOL fetch_value(const char* addr, unsigned sz, int* value)
{
    char        value8;
    short       value16;

    switch (sz)
    {
    case 8:
        if (!dbg_read_memory(addr, &value8, sizeof(value8))) return FALSE;
        *value = value8;
        break;
    case 16:
        if (!dbg_read_memory(addr, &value16, sizeof(value16))) return FALSE;
        *value = value16;
    case 32:
        if (!dbg_read_memory(addr, value, sizeof(*value))) return FALSE;
        break;
    default: return FALSE;
    }
    return TRUE;
}

static BOOL be_x86_64_is_func_call(const void* insn, ADDRESS64* callee)
{
    BYTE                ch;
    LONG                delta;
    unsigned            op_size = 32, rex = 0;
    DWORD64             dst;

    /* we assume 64bit mode all over the place */
    for (;;)
    {
        if (!dbg_read_memory(insn, &ch, sizeof(ch))) return FALSE;
        if (ch == 0x66) op_size = 16;
        else if (ch == 0x67) WINE_FIXME("prefix not supported %x\n", ch);
        else if (ch >= 0x40 && ch <= 0x4f) rex = ch & 0xf;
        else break;
        insn = (const char*)insn + 1;
    } while (0);

    /* that's the only mode we support anyway */
    callee->Mode = AddrModeFlat;
    callee->Segment = dbg_context.SegCs;

    switch (ch)
    {
    case 0xe8: /* relative near call */
        assert(op_size == 32);
        if (!fetch_value((const char*)insn + 1, sizeof(delta), &delta))
            return FALSE;
        callee->Offset = (DWORD_PTR)insn + 1 + 4 + delta;
        return TRUE;

    case 0xff:
        if (!dbg_read_memory((const char*)insn + 1, &ch, sizeof(ch)))
            return FALSE;
        WINE_TRACE("Got 0xFF %x (&C7=%x) with rex=%x\n", ch, ch & 0xC7, rex);
        /* keep only the CALL and LCALL insn:s */
        switch (f_reg(ch))
        {
        case 0x02:
            break;
        default: return FALSE;
        }
        if (rex == 0) switch (ch & 0xC7) /* keep Mod R/M only (skip reg) */
        {
        case 0x04:
        case 0x44:
        case 0x84:
            WINE_FIXME("Unsupported yet call insn (0xFF 0x%02x) (SIB bytes) at %p\n", ch, insn);
            return FALSE;
        case 0x05: /* addr32 */
            if (f_reg(ch) == 0x2)
            {
                /* rip-relative to next insn */
                if (!dbg_read_memory((const char*)insn + 2, &delta, sizeof(delta)) ||
                    !dbg_read_memory((const char*)insn + 6 + delta, &dst, sizeof(dst)))
                    return FALSE;

                callee->Offset = dst;
                return TRUE;
            }
            WINE_FIXME("Unsupported yet call insn (0xFF 0x%02x) at %p\n", ch, insn);
            return FALSE;
        default:
            switch (f_rm(ch))
            {
            case 0x00: dst = dbg_context.Rax; break;
            case 0x01: dst = dbg_context.Rcx; break;
            case 0x02: dst = dbg_context.Rdx; break;
            case 0x03: dst = dbg_context.Rbx; break;
            case 0x04: dst = dbg_context.Rsp; break;
            case 0x05: dst = dbg_context.Rbp; break;
            case 0x06: dst = dbg_context.Rsi; break;
            case 0x07: dst = dbg_context.Rdi; break;
            }
            if (f_mod(ch) != 0x03)
                WINE_FIXME("Unsupported yet call insn (0xFF 0x%02x) at %p\n", ch, insn);
            else
            {
                callee->Offset = dst;
            }
            break;
        }
        else
            WINE_FIXME("Unsupported yet call insn (rex=0x%02x 0xFF 0x%02x) at %p\n", rex, ch, insn);
        return FALSE;

    default:
        return FALSE;
    }
}

static BOOL be_x86_64_is_jump(const void* insn, ADDRESS64* jumpee)
{
    return FALSE;
}

extern void be_x86_64_disasm_one_insn(ADDRESS64* addr, int display);

#define DR7_CONTROL_SHIFT	16
#define DR7_CONTROL_SIZE 	4

#define DR7_RW_EXECUTE 		(0x0)
#define DR7_RW_WRITE		(0x1)
#define DR7_RW_READ		(0x3)

#define DR7_LEN_1		(0x0)
#define DR7_LEN_2		(0x4)
#define DR7_LEN_4		(0xC)
#define DR7_LEN_8		(0x8)

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

static BOOL be_x86_64_insert_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
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
        if (size != 0) return FALSE;
        if (!pio->read(hProcess, addr, &ch, 1, &sz) || sz != 1) return FALSE;
        *val = ch;
        ch = 0xcc;
        if (!pio->write(hProcess, addr, &ch, 1, &sz) || sz != 1) return FALSE;
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
        if ((reg = be_x86_64_get_unused_DR(ctx, &pr)) == -1) return FALSE;
        *pr = (DWORD64)addr;
        if (type != be_xpoint_watch_exec) switch (size)
        {
        case 8: bits |= DR7_LEN_8; break;
        case 4: bits |= DR7_LEN_4; break;
        case 2: bits |= DR7_LEN_2; break;
        case 1: bits |= DR7_LEN_1; break;
        default: WINE_FIXME("Unsupported xpoint_watch of size %d\n", size); return FALSE;
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
        return FALSE;
    }
    return TRUE;
}

static BOOL be_x86_64_remove_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                    CONTEXT* ctx, enum be_xpoint_type type,
                                    void* addr, unsigned long val, unsigned size)
{
    SIZE_T              sz;
    unsigned char       ch;

    switch (type)
    {
    case be_xpoint_break:
        if (size != 0) return FALSE;
        if (!pio->read(hProcess, addr, &ch, 1, &sz) || sz != 1) return FALSE;
        if (ch != (unsigned char)0xCC)
            WINE_FIXME("Cannot get back %02x instead of 0xCC at %p\n", ch, addr);
        ch = (unsigned char)val;
        if (!pio->write(hProcess, addr, &ch, 1, &sz) || sz != 1) return FALSE;
        break;
    case be_xpoint_watch_exec:
    case be_xpoint_watch_read:
    case be_xpoint_watch_write:
        /* simply disable the entry */
        ctx->Dr7 &= ~DR7_ENABLE_MASK(val);
        break;
    default:
        dbg_printf("Unknown bp type %c\n", type);
        return FALSE;
    }
    return TRUE;
}

static BOOL be_x86_64_is_watchpoint_set(const CONTEXT* ctx, unsigned idx)
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

static BOOL be_x86_64_fetch_integer(const struct dbg_lvalue* lvalue, unsigned size,
                                    BOOL is_signed, LONGLONG* ret)
{
    if (size != 1 && size != 2 && size != 4 && size != 8 && size != 16)
        return FALSE;

    memset(ret, 0, sizeof(*ret)); /* clear unread bytes */
    /* FIXME: this assumes that debuggee and debugger use the same
     * integral representation
     */
    if (!memory_read_value(lvalue, size, ret)) return FALSE;

    /* propagate sign information */
    if (is_signed && size < 16 && (*ret >> (size * 8 - 1)) != 0)
    {
        ULONGLONG neg = -1;
        *ret |= neg << (size * 8);
    }
    return TRUE;
}

static BOOL be_x86_64_fetch_float(const struct dbg_lvalue* lvalue, unsigned size,
                                  long double* ret)
{
    char        tmp[sizeof(long double)];

    /* FIXME: this assumes that debuggee and debugger use the same
     * representation for reals
     */
    if (!memory_read_value(lvalue, size, tmp)) return FALSE;

    /* float & double types have to be promoted to a long double */
    if (size == sizeof(float)) *ret = *(float*)tmp;
    else if (size == sizeof(double)) *ret = *(double*)tmp;
    else if (size == sizeof(long double)) *ret = *(long double*)tmp;
    else return FALSE;

    return TRUE;
}

static BOOL be_x86_64_store_integer(const struct dbg_lvalue* lvalue, unsigned size,
                                    BOOL is_signed, LONGLONG val)
{
    /* this is simple as we're on a little endian CPU */
    return memory_write_value(lvalue, size, &val);
}

struct backend_cpu be_x86_64 =
{
    IMAGE_FILE_MACHINE_AMD64,
    8,
    be_cpu_linearize,
    be_cpu_build_addr,
    be_x86_64_get_addr,
    be_x86_64_get_register_info,
    be_x86_64_single_step,
    be_x86_64_print_context,
    be_x86_64_print_segment_info,
    be_x86_64_ctx,
    be_x86_64_is_step_over_insn,
    be_x86_64_is_function_return,
    be_x86_64_is_break_insn,
    be_x86_64_is_func_call,
    be_x86_64_is_jump,
    be_x86_64_disasm_one_insn,
    be_x86_64_insert_Xpoint,
    be_x86_64_remove_Xpoint,
    be_x86_64_is_watchpoint_set,
    be_x86_64_clear_watchpoint,
    be_x86_64_adjust_pc_for_break,
    be_x86_64_fetch_integer,
    be_x86_64_fetch_float,
    be_x86_64_store_integer,
};
#endif
