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

#if defined(__x86_64__)

WINE_DEFAULT_DEBUG_CHANNEL(winedbg);

#define STEP_FLAG 0x00000100 /* single step flag */

static BOOL be_x86_64_get_addr(HANDLE hThread, const dbg_ctx_t *ctx,
                               enum be_cpu_addr bca, ADDRESS64* addr)
{
    addr->Mode = AddrModeFlat;
    switch (bca)
    {
    case be_cpu_addr_pc:
        addr->Segment = ctx->ctx.SegCs;
        addr->Offset = ctx->ctx.Rip;
        return TRUE;
    case be_cpu_addr_stack:
        addr->Segment = ctx->ctx.SegSs;
        addr->Offset = ctx->ctx.Rsp;
        return TRUE;
    case be_cpu_addr_frame:
        addr->Segment = ctx->ctx.SegSs;
        addr->Offset = ctx->ctx.Rbp;
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

static void be_x86_64_single_step(dbg_ctx_t *ctx, BOOL enable)
{
    if (enable) ctx->ctx.EFlags |= STEP_FLAG;
    else ctx->ctx.EFlags &= ~STEP_FLAG;
}

static void be_x86_64_print_context(HANDLE hThread, const dbg_ctx_t *pctx,
                                    int all_regs)
{
    static const char mxcsr_flags[16][4] = { "IE", "DE", "ZE", "OE", "UE", "PE", "DAZ", "IM",
                                             "DM", "ZM", "OM", "UM", "PM", "R-", "R+", "FZ" };
    static const char flags[] = "aVR-N--ODITSZ-A-P-C";
    const CONTEXT *ctx = &pctx->ctx;
    char buf[33];
    int i;

    strcpy(buf, flags);
    for (i = 0; buf[i]; i++)
        if (buf[i] != '-' && !(ctx->EFlags & (1 << (sizeof(flags) - 2 - i))))
            buf[i] = ' ';

    dbg_printf("Register dump:\n");
    dbg_printf(" rip:%016I64x rsp:%016I64x rbp:%016I64x eflags:%08lx (%s)\n",
               ctx->Rip, ctx->Rsp, ctx->Rbp, ctx->EFlags, buf);
    dbg_printf(" rax:%016I64x rbx:%016I64x rcx:%016I64x rdx:%016I64x\n",
               ctx->Rax, ctx->Rbx, ctx->Rcx, ctx->Rdx);
    dbg_printf(" rsi:%016I64x rdi:%016I64x  r8:%016I64x  r9:%016I64x r10:%016I64x\n",
               ctx->Rsi, ctx->Rdi, ctx->R8, ctx->R9, ctx->R10 );
    dbg_printf(" r11:%016I64x r12:%016I64x r13:%016I64x r14:%016I64x r15:%016I64x\n",
               ctx->R11, ctx->R12, ctx->R13, ctx->R14, ctx->R15 );

    if (!all_regs) return;

    dbg_printf("  cs:%04x  ds:%04x  es:%04x  fs:%04x  gs:%04x  ss:%04x\n",
               ctx->SegCs, ctx->SegDs, ctx->SegEs, ctx->SegFs, ctx->SegGs, ctx->SegSs );

    dbg_printf("Debug:\n");
    dbg_printf(" dr0:%016I64x dr1:%016I64x dr2:%016I64x dr3:%016I64x\n",
               ctx->Dr0, ctx->Dr1, ctx->Dr2, ctx->Dr3 );
    dbg_printf(" dr6:%016I64x dr7:%016I64x\n", ctx->Dr6, ctx->Dr7 );

    dbg_printf("Floating point:\n");
    dbg_printf(" flcw:%04x ", LOWORD(ctx->FltSave.ControlWord));
    dbg_printf(" fltw:%04x ", LOWORD(ctx->FltSave.TagWord));
    dbg_printf(" flsw:%04x", LOWORD(ctx->FltSave.StatusWord));

    dbg_printf("(cc:%d%d%d%d", (ctx->FltSave.StatusWord & 0x00004000) >> 14,
               (ctx->FltSave.StatusWord & 0x00000400) >> 10,
               (ctx->FltSave.StatusWord & 0x00000200) >> 9,
               (ctx->FltSave.StatusWord & 0x00000100) >> 8);

    dbg_printf(" top:%01x", (unsigned int) (ctx->FltSave.StatusWord & 0x00003800) >> 11);

    if (ctx->FltSave.StatusWord & 0x00000001)     /* Invalid Fl OP */
    {
       if (ctx->FltSave.StatusWord & 0x00000040)  /* Stack Fault */
       {
          if (ctx->FltSave.StatusWord & 0x00000200) /* C1 says Overflow */
             dbg_printf(" #IE(Stack Overflow)");
          else
             dbg_printf(" #IE(Stack Underflow)");     /* Underflow */
       }
       else  dbg_printf(" #IE(Arithmetic error)");    /* Invalid Fl OP */
    }
    if (ctx->FltSave.StatusWord & 0x00000002) dbg_printf(" #DE"); /* Denormalised OP */
    if (ctx->FltSave.StatusWord & 0x00000004) dbg_printf(" #ZE"); /* Zero Divide */
    if (ctx->FltSave.StatusWord & 0x00000008) dbg_printf(" #OE"); /* Overflow */
    if (ctx->FltSave.StatusWord & 0x00000010) dbg_printf(" #UE"); /* Underflow */
    if (ctx->FltSave.StatusWord & 0x00000020) dbg_printf(" #PE"); /* Precision error */
    if (ctx->FltSave.StatusWord & 0x00000040)
       if (!(ctx->FltSave.StatusWord & 0x00000001))
           dbg_printf(" #SE");                 /* Stack Fault (don't think this can occur) */
    if (ctx->FltSave.StatusWord & 0x00000080) dbg_printf(" #ES"); /* Error Summary */
    if (ctx->FltSave.StatusWord & 0x00008000) dbg_printf(" #FB"); /* FPU Busy */
    dbg_printf(")\n");
    dbg_printf(" flerr:%04x:%08lx   fldata:%04x:%08lx\n",
               ctx->FltSave.ErrorSelector, ctx->FltSave.ErrorOffset,
               ctx->FltSave.DataSelector, ctx->FltSave.DataOffset );

    for (i = 0; i < 8; i++)
    {
        M128A reg = ctx->FltSave.FloatRegisters[i];
        if (i == 4) dbg_printf("\n");
        dbg_printf(" ST%u:%016I64x%16I64x ", i, reg.High, reg.Low );
    }
    dbg_printf("\n");

    dbg_printf(" mxcsr: %04lx (", ctx->FltSave.MxCsr );
    for (i = 0; i < 16; i++)
        if (ctx->FltSave.MxCsr & (1 << i)) dbg_printf( " %s", mxcsr_flags[i] );
    dbg_printf(" )\n");

    for (i = 0; i < 16; i++)
    {
        dbg_printf( " %sxmm%u: uint=%016I64x%016I64x", (i > 9) ? "" : " ", i,
                    ctx->FltSave.XmmRegisters[i].High, ctx->FltSave.XmmRegisters[i].Low );
        dbg_printf( " double={%g; %g}", *(double *)&ctx->FltSave.XmmRegisters[i].Low,
                    *(double *)&ctx->FltSave.XmmRegisters[i].High );
        dbg_printf( " float={%g; %g; %g; %g}\n",
                    (double)*((float *)&ctx->FltSave.XmmRegisters[i] + 0),
                    (double)*((float *)&ctx->FltSave.XmmRegisters[i] + 1),
                    (double)*((float *)&ctx->FltSave.XmmRegisters[i] + 2),
                    (double)*((float *)&ctx->FltSave.XmmRegisters[i] + 3) );
    }
}

static void be_x86_64_print_segment_info(HANDLE hThread, const dbg_ctx_t *ctx)
{
}

static struct dbg_internal_var be_x86_64_ctx[] =
{
    {CV_AMD64_AL,       "AL",           (void*)FIELD_OFFSET(CONTEXT, Rax),     dbg_itype_unsigned_int8},
    {CV_AMD64_BL,       "BL",           (void*)FIELD_OFFSET(CONTEXT, Rbx),     dbg_itype_unsigned_int8},
    {CV_AMD64_CL,       "CL",           (void*)FIELD_OFFSET(CONTEXT, Rcx),     dbg_itype_unsigned_int8},
    {CV_AMD64_DL,       "DL",           (void*)FIELD_OFFSET(CONTEXT, Rdx),     dbg_itype_unsigned_int8},
    {CV_AMD64_AH,       "AH",           (void*)(FIELD_OFFSET(CONTEXT, Rax)+1), dbg_itype_unsigned_int8},
    {CV_AMD64_BH,       "BH",           (void*)(FIELD_OFFSET(CONTEXT, Rbx)+1), dbg_itype_unsigned_int8},
    {CV_AMD64_CH,       "CH",           (void*)(FIELD_OFFSET(CONTEXT, Rcx)+1), dbg_itype_unsigned_int8},
    {CV_AMD64_DH,       "DH",           (void*)(FIELD_OFFSET(CONTEXT, Rdx)+1), dbg_itype_unsigned_int8},
    {CV_AMD64_AX,       "AX",           (void*)FIELD_OFFSET(CONTEXT, Rax),     dbg_itype_unsigned_int16},
    {CV_AMD64_BX,       "BX",           (void*)FIELD_OFFSET(CONTEXT, Rbx),     dbg_itype_unsigned_int16},
    {CV_AMD64_CX,       "CX",           (void*)FIELD_OFFSET(CONTEXT, Rcx),     dbg_itype_unsigned_int16},
    {CV_AMD64_DX,       "DX",           (void*)FIELD_OFFSET(CONTEXT, Rdx),     dbg_itype_unsigned_int16},
    {CV_AMD64_SP,       "SP",           (void*)FIELD_OFFSET(CONTEXT, Rsp),     dbg_itype_unsigned_int16},
    {CV_AMD64_BP,       "BP",           (void*)FIELD_OFFSET(CONTEXT, Rbp),     dbg_itype_unsigned_int16},
    {CV_AMD64_SI,       "SI",           (void*)FIELD_OFFSET(CONTEXT, Rsi),     dbg_itype_unsigned_int16},
    {CV_AMD64_DI,       "DI",           (void*)FIELD_OFFSET(CONTEXT, Rdi),     dbg_itype_unsigned_int16},
    {CV_AMD64_EAX,      "EAX",          (void*)FIELD_OFFSET(CONTEXT, Rax),     dbg_itype_unsigned_int32},
    {CV_AMD64_EBX,      "EBX",          (void*)FIELD_OFFSET(CONTEXT, Rbx),     dbg_itype_unsigned_int32},
    {CV_AMD64_ECX,      "ECX",          (void*)FIELD_OFFSET(CONTEXT, Rcx),     dbg_itype_unsigned_int32},
    {CV_AMD64_EDX,      "EDX",          (void*)FIELD_OFFSET(CONTEXT, Rdx),     dbg_itype_unsigned_int32},
    {CV_AMD64_ESP,      "ESP",          (void*)FIELD_OFFSET(CONTEXT, Rsp),     dbg_itype_unsigned_int32},
    {CV_AMD64_EBP,      "EBP",          (void*)FIELD_OFFSET(CONTEXT, Rbp),     dbg_itype_unsigned_int32},
    {CV_AMD64_ESI,      "ESI",          (void*)FIELD_OFFSET(CONTEXT, Rsi),     dbg_itype_unsigned_int32},
    {CV_AMD64_EDI,      "EDI",          (void*)FIELD_OFFSET(CONTEXT, Rdi),     dbg_itype_unsigned_int32},
    {CV_AMD64_ES,       "ES",           (void*)FIELD_OFFSET(CONTEXT, SegEs),   dbg_itype_unsigned_int16},
    {CV_AMD64_CS,       "CS",           (void*)FIELD_OFFSET(CONTEXT, SegCs),   dbg_itype_unsigned_int16},
    {CV_AMD64_SS,       "SS",           (void*)FIELD_OFFSET(CONTEXT, SegSs),   dbg_itype_unsigned_int16},
    {CV_AMD64_DS,       "DS",           (void*)FIELD_OFFSET(CONTEXT, SegDs),   dbg_itype_unsigned_int16},
    {CV_AMD64_FS,       "FS",           (void*)FIELD_OFFSET(CONTEXT, SegFs),   dbg_itype_unsigned_int16},
    {CV_AMD64_GS,       "GS",           (void*)FIELD_OFFSET(CONTEXT, SegGs),   dbg_itype_unsigned_int16},
    {CV_AMD64_FLAGS,    "FLAGS",        (void*)FIELD_OFFSET(CONTEXT, EFlags),  dbg_itype_unsigned_int16},
    {CV_AMD64_EFLAGS,   "EFLAGS",       (void*)FIELD_OFFSET(CONTEXT, EFlags),  dbg_itype_unsigned_int32},
    {CV_AMD64_RIP,      "RIP",          (void*)FIELD_OFFSET(CONTEXT, Rip),     dbg_itype_unsigned_int64},
    {CV_AMD64_RAX,      "RAX",          (void*)FIELD_OFFSET(CONTEXT, Rax),     dbg_itype_unsigned_int64},
    {CV_AMD64_RBX,      "RBX",          (void*)FIELD_OFFSET(CONTEXT, Rbx),     dbg_itype_unsigned_int64},
    {CV_AMD64_RCX,      "RCX",          (void*)FIELD_OFFSET(CONTEXT, Rcx),     dbg_itype_unsigned_int64},
    {CV_AMD64_RDX,      "RDX",          (void*)FIELD_OFFSET(CONTEXT, Rdx),     dbg_itype_unsigned_int64},
    {CV_AMD64_RSP,      "RSP",          (void*)FIELD_OFFSET(CONTEXT, Rsp),     dbg_itype_unsigned_int64},
    {CV_AMD64_RBP,      "RBP",          (void*)FIELD_OFFSET(CONTEXT, Rbp),     dbg_itype_unsigned_int64},
    {CV_AMD64_RSI,      "RSI",          (void*)FIELD_OFFSET(CONTEXT, Rsi),     dbg_itype_unsigned_int64},
    {CV_AMD64_RDI,      "RDI",          (void*)FIELD_OFFSET(CONTEXT, Rdi),     dbg_itype_unsigned_int64},
    {CV_AMD64_R8,       "R8",           (void*)FIELD_OFFSET(CONTEXT, R8),      dbg_itype_unsigned_int64},
    {CV_AMD64_R9,       "R9",           (void*)FIELD_OFFSET(CONTEXT, R9),      dbg_itype_unsigned_int64},
    {CV_AMD64_R10,      "R10",          (void*)FIELD_OFFSET(CONTEXT, R10),     dbg_itype_unsigned_int64},
    {CV_AMD64_R11,      "R11",          (void*)FIELD_OFFSET(CONTEXT, R11),     dbg_itype_unsigned_int64},
    {CV_AMD64_R12,      "R12",          (void*)FIELD_OFFSET(CONTEXT, R12),     dbg_itype_unsigned_int64},
    {CV_AMD64_R13,      "R13",          (void*)FIELD_OFFSET(CONTEXT, R13),     dbg_itype_unsigned_int64},
    {CV_AMD64_R14,      "R14",          (void*)FIELD_OFFSET(CONTEXT, R14),     dbg_itype_unsigned_int64},
    {CV_AMD64_R15,      "R15",          (void*)FIELD_OFFSET(CONTEXT, R15),     dbg_itype_unsigned_int64},
    {CV_AMD64_ST0,      "ST0",          (void*)FIELD_OFFSET(CONTEXT, FltSave.FloatRegisters[0]), dbg_itype_long_real},
    {CV_AMD64_ST0+1,    "ST1",          (void*)FIELD_OFFSET(CONTEXT, FltSave.FloatRegisters[1]), dbg_itype_long_real},
    {CV_AMD64_ST0+2,    "ST2",          (void*)FIELD_OFFSET(CONTEXT, FltSave.FloatRegisters[2]), dbg_itype_long_real},
    {CV_AMD64_ST0+3,    "ST3",          (void*)FIELD_OFFSET(CONTEXT, FltSave.FloatRegisters[3]), dbg_itype_long_real},
    {CV_AMD64_ST0+4,    "ST4",          (void*)FIELD_OFFSET(CONTEXT, FltSave.FloatRegisters[4]), dbg_itype_long_real},
    {CV_AMD64_ST0+5,    "ST5",          (void*)FIELD_OFFSET(CONTEXT, FltSave.FloatRegisters[5]), dbg_itype_long_real},
    {CV_AMD64_ST0+6,    "ST6",          (void*)FIELD_OFFSET(CONTEXT, FltSave.FloatRegisters[6]), dbg_itype_long_real},
    {CV_AMD64_ST0+7,    "ST7",          (void*)FIELD_OFFSET(CONTEXT, FltSave.FloatRegisters[7]), dbg_itype_long_real},
    {CV_AMD64_XMM0,     "XMM0",         (void*)FIELD_OFFSET(CONTEXT, Xmm0),  dbg_itype_m128a},
    {CV_AMD64_XMM0+1,   "XMM1",         (void*)FIELD_OFFSET(CONTEXT, Xmm1),  dbg_itype_m128a},
    {CV_AMD64_XMM0+2,   "XMM2",         (void*)FIELD_OFFSET(CONTEXT, Xmm2),  dbg_itype_m128a},
    {CV_AMD64_XMM0+3,   "XMM3",         (void*)FIELD_OFFSET(CONTEXT, Xmm3),  dbg_itype_m128a},
    {CV_AMD64_XMM0+4,   "XMM4",         (void*)FIELD_OFFSET(CONTEXT, Xmm4),  dbg_itype_m128a},
    {CV_AMD64_XMM0+5,   "XMM5",         (void*)FIELD_OFFSET(CONTEXT, Xmm5),  dbg_itype_m128a},
    {CV_AMD64_XMM0+6,   "XMM6",         (void*)FIELD_OFFSET(CONTEXT, Xmm6),  dbg_itype_m128a},
    {CV_AMD64_XMM0+7,   "XMM7",         (void*)FIELD_OFFSET(CONTEXT, Xmm7),  dbg_itype_m128a},
    {CV_AMD64_XMM8,     "XMM8",         (void*)FIELD_OFFSET(CONTEXT, Xmm8),  dbg_itype_m128a},
    {CV_AMD64_XMM8+1,   "XMM9",         (void*)FIELD_OFFSET(CONTEXT, Xmm9),  dbg_itype_m128a},
    {CV_AMD64_XMM8+2,   "XMM10",        (void*)FIELD_OFFSET(CONTEXT, Xmm10), dbg_itype_m128a},
    {CV_AMD64_XMM8+3,   "XMM11",        (void*)FIELD_OFFSET(CONTEXT, Xmm11), dbg_itype_m128a},
    {CV_AMD64_XMM8+4,   "XMM12",        (void*)FIELD_OFFSET(CONTEXT, Xmm12), dbg_itype_m128a},
    {CV_AMD64_XMM8+5,   "XMM13",        (void*)FIELD_OFFSET(CONTEXT, Xmm13), dbg_itype_m128a},
    {CV_AMD64_XMM8+6,   "XMM14",        (void*)FIELD_OFFSET(CONTEXT, Xmm14), dbg_itype_m128a},
    {CV_AMD64_XMM8+7,   "XMM15",        (void*)FIELD_OFFSET(CONTEXT, Xmm15), dbg_itype_m128a},
    {0,                 NULL,           0,                                   dbg_itype_none}
};

#define	f_mod(b)	((b)>>6)
#define	f_reg(b)	(((b)>>3)&0x7)
#define	f_rm(b)		((b)&0x7)
#define	f_sib_b(b)	((b)&0x7)
#define	f_sib_i(b)	(((b)>>3)&0x7)
#define	f_sib_s(b)	((b)>>6)

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

static BOOL fetch_value(const char* addr, unsigned sz, LONG* value)
{
    char        value8;
    short       value16;

    switch (sz)
    {
    case 1:
        if (!dbg_read_memory(addr, &value8, sizeof(value8))) return FALSE;
        *value = value8;
        break;
    case 2:
        if (!dbg_read_memory(addr, &value16, sizeof(value16))) return FALSE;
        *value = value16;
        break;
    case 4:
        if (!dbg_read_memory(addr, value, sizeof(*value))) return FALSE;
        break;
    default: return FALSE;
    }
    return TRUE;
}

static BOOL add_fixed_displacement(const void* insn, BYTE mod, DWORD64* addr)
{
    LONG delta = 0;

    if (mod == 1)
    {
        if (!fetch_value(insn, 1, &delta))
            return FALSE;
    }
    else if (mod == 2)
    {
        if (!fetch_value(insn, sizeof(delta), &delta))
            return FALSE;
    }
    *addr += delta;
    return TRUE;
}

static BOOL evaluate_sib_address(const void* insn, BYTE mod, DWORD64* addr)
{
    BYTE    ch;
    BYTE    scale;
    DWORD64 loc;

    if (!dbg_read_memory(insn, &ch, sizeof(ch))) return FALSE;

    switch (f_sib_b(ch))
    {
    case 0x00: loc = dbg_context.ctx.Rax; break;
    case 0x01: loc = dbg_context.ctx.Rcx; break;
    case 0x02: loc = dbg_context.ctx.Rdx; break;
    case 0x03: loc = dbg_context.ctx.Rbx; break;
    case 0x04: loc = dbg_context.ctx.Rsp; break;
    case 0x05:
        loc = dbg_context.ctx.Rbp;
        if (mod == 0)
        {
            loc = 0;
            mod = 2;
        }
        break;
    case 0x06: loc = dbg_context.ctx.Rsi; break;
    case 0x07: loc = dbg_context.ctx.Rdi; break;
    }

    scale = f_sib_s(ch);
    switch (f_sib_i(ch))
    {
    case 0x00: loc += dbg_context.ctx.Rax << scale; break;
    case 0x01: loc += dbg_context.ctx.Rcx << scale; break;
    case 0x02: loc += dbg_context.ctx.Rdx << scale; break;
    case 0x03: loc += dbg_context.ctx.Rbx << scale; break;
    case 0x04: break;
    case 0x05: loc += dbg_context.ctx.Rbp << scale; break;
    case 0x06: loc += dbg_context.ctx.Rsi << scale; break;
    case 0x07: loc += dbg_context.ctx.Rdi << scale; break;
    }

    if (!add_fixed_displacement((const char*)insn + 1, mod, &loc))
        return FALSE;

    *addr = loc;
    return TRUE;
}

static BOOL load_indirect_target(DWORD64* dst)
{
    ADDRESS64 addr;

    addr.Mode = AddrModeFlat;
    addr.Segment = dbg_context.ctx.SegDs;
    addr.Offset = *dst;
    return dbg_read_memory(memory_to_linear_addr(&addr), &dst, sizeof(dst));
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
    callee->Segment = dbg_context.ctx.SegCs;

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
        {
            evaluate_sib_address((const char*)insn + 2, f_mod(ch), &dst);
            if (!load_indirect_target(&dst)) return FALSE;
            callee->Offset = dst;
            return TRUE;
        }
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
            case 0x00: dst = dbg_context.ctx.Rax; break;
            case 0x01: dst = dbg_context.ctx.Rcx; break;
            case 0x02: dst = dbg_context.ctx.Rdx; break;
            case 0x03: dst = dbg_context.ctx.Rbx; break;
            case 0x04: dst = dbg_context.ctx.Rsp; break;
            case 0x05: dst = dbg_context.ctx.Rbp; break;
            case 0x06: dst = dbg_context.ctx.Rsi; break;
            case 0x07: dst = dbg_context.ctx.Rdi; break;
            }
            if (f_mod(ch) != 0x03)
            {
                if (!add_fixed_displacement((const char*)insn + 2, f_mod(ch), &dst))
                    return FALSE;
                if (!load_indirect_target(&dst)) return FALSE;
            }
            callee->Offset = dst;
            return TRUE;
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

static inline int be_x86_64_get_unused_DR(dbg_ctx_t *pctx, DWORD64** r)
{
    CONTEXT *ctx = &pctx->ctx;

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
                                    dbg_ctx_t *ctx, enum be_xpoint_type type,
                                    void* addr, unsigned *val, unsigned size)
{
    unsigned char       ch;
    SIZE_T              sz;
    DWORD64            *pr;
    int                 reg;
    unsigned int        bits;

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
        ctx->ctx.Dr7 &= ~(0x0F << (DR7_CONTROL_SHIFT + DR7_CONTROL_SIZE * reg));
        /* set the correct ones */
        ctx->ctx.Dr7 |= bits << (DR7_CONTROL_SHIFT + DR7_CONTROL_SIZE * reg);
        ctx->ctx.Dr7 |= DR7_ENABLE_MASK(reg) | DR7_LOCAL_SLOWDOWN;
        break;
    default:
        dbg_printf("Unknown bp type %c\n", type);
        return FALSE;
    }
    return TRUE;
}

static BOOL be_x86_64_remove_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                    dbg_ctx_t *ctx, enum be_xpoint_type type,
                                    void* addr, unsigned val, unsigned size)
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
        ctx->ctx.Dr7 &= ~DR7_ENABLE_MASK(val);
        break;
    default:
        dbg_printf("Unknown bp type %c\n", type);
        return FALSE;
    }
    return TRUE;
}

static BOOL be_x86_64_is_watchpoint_set(const dbg_ctx_t *ctx, unsigned idx)
{
    return ctx->ctx.Dr6 & (1 << idx);
}

static void be_x86_64_clear_watchpoint(dbg_ctx_t *ctx, unsigned idx)
{
    ctx->ctx.Dr6 &= ~(1 << idx);
}

static int be_x86_64_adjust_pc_for_break(dbg_ctx_t *ctx, BOOL way)
{
    if (way)
    {
        ctx->ctx.Rip--;
        return -1;
    }
    ctx->ctx.Rip++;
    return 1;
}

static BOOL be_x86_64_get_context(HANDLE thread, dbg_ctx_t *ctx)
{
    ctx->ctx.ContextFlags = CONTEXT_ALL;
    return GetThreadContext(thread, &ctx->ctx);
}

static BOOL be_x86_64_set_context(HANDLE thread, const dbg_ctx_t *ctx)
{
    return SetThreadContext(thread, &ctx->ctx);
}

#define REG(f,n,t,r)  {f, n, t, FIELD_OFFSET(CONTEXT, r), sizeof(((CONTEXT*)NULL)->r)}

static struct gdb_register be_x86_64_gdb_register_map[] = {
    REG("core", "rax",    NULL,          Rax),
    REG(NULL,   "rbx",    NULL,          Rbx),
    REG(NULL,   "rcx",    NULL,          Rcx),
    REG(NULL,   "rdx",    NULL,          Rdx),
    REG(NULL,   "rsi",    NULL,          Rsi),
    REG(NULL,   "rdi",    NULL,          Rdi),
    REG(NULL,   "rbp",    "data_ptr",    Rbp),
    REG(NULL,   "rsp",    "data_ptr",    Rsp),
    REG(NULL,   "r8",     NULL,          R8),
    REG(NULL,   "r9",     NULL,          R9),
    REG(NULL,   "r10",    NULL,          R10),
    REG(NULL,   "r11",    NULL,          R11),
    REG(NULL,   "r12",    NULL,          R12),
    REG(NULL,   "r13",    NULL,          R13),
    REG(NULL,   "r14",    NULL,          R14),
    REG(NULL,   "r15",    NULL,          R15),
    REG(NULL,   "rip",    "code_ptr",    Rip),
    REG(NULL,   "eflags", "i386_eflags", EFlags),
    REG(NULL,   "cs",     NULL,          SegCs),
    REG(NULL,   "ss",     NULL,          SegSs),
    REG(NULL,   "ds",     NULL,          SegDs),
    REG(NULL,   "es",     NULL,          SegEs),
    REG(NULL,   "fs",     NULL,          SegFs),
    REG(NULL,   "gs",     NULL,          SegGs),
    { NULL,     "st0",    "i387_ext",    FIELD_OFFSET(CONTEXT, FltSave.FloatRegisters[ 0]), 10},
    { NULL,     "st1",    "i387_ext",    FIELD_OFFSET(CONTEXT, FltSave.FloatRegisters[ 1]), 10},
    { NULL,     "st2",    "i387_ext",    FIELD_OFFSET(CONTEXT, FltSave.FloatRegisters[ 2]), 10},
    { NULL,     "st3",    "i387_ext",    FIELD_OFFSET(CONTEXT, FltSave.FloatRegisters[ 3]), 10},
    { NULL,     "st4",    "i387_ext",    FIELD_OFFSET(CONTEXT, FltSave.FloatRegisters[ 4]), 10},
    { NULL,     "st5",    "i387_ext",    FIELD_OFFSET(CONTEXT, FltSave.FloatRegisters[ 5]), 10},
    { NULL,     "st6",    "i387_ext",    FIELD_OFFSET(CONTEXT, FltSave.FloatRegisters[ 6]), 10},
    { NULL,     "st7",    "i387_ext",    FIELD_OFFSET(CONTEXT, FltSave.FloatRegisters[ 7]), 10},
    REG(NULL,   "fctrl",  NULL,          FltSave.ControlWord),
    REG(NULL,   "fstat",  NULL,          FltSave.StatusWord),
    REG(NULL,   "ftag",   NULL,          FltSave.TagWord),
    REG(NULL,   "fiseg",  NULL,          FltSave.ErrorSelector),
    REG(NULL,   "fioff",  NULL,          FltSave.ErrorOffset),
    REG(NULL,   "foseg",  NULL,          FltSave.DataSelector),
    REG(NULL,   "fooff",  NULL,          FltSave.DataOffset),
    REG(NULL,   "fop",    NULL,          FltSave.ErrorOpcode),

    REG("sse",  "xmm0",    "vec128",     Xmm0),
    REG(NULL,   "xmm1",    "vec128",     Xmm1),
    REG(NULL,   "xmm2",    "vec128",     Xmm2),
    REG(NULL,   "xmm3",    "vec128",     Xmm3),
    REG(NULL,   "xmm4",    "vec128",     Xmm4),
    REG(NULL,   "xmm5",    "vec128",     Xmm5),
    REG(NULL,   "xmm6",    "vec128",     Xmm6),
    REG(NULL,   "xmm7",    "vec128",     Xmm7),
    REG(NULL,   "xmm8",    "vec128",     Xmm8),
    REG(NULL,   "xmm9",    "vec128",     Xmm9),
    REG(NULL,   "xmm10",   "vec128",     Xmm10),
    REG(NULL,   "xmm11",   "vec128",     Xmm11),
    REG(NULL,   "xmm12",   "vec128",     Xmm12),
    REG(NULL,   "xmm13",   "vec128",     Xmm13),
    REG(NULL,   "xmm14",   "vec128",     Xmm14),
    REG(NULL,   "xmm15",   "vec128",     Xmm15),
    REG(NULL,   "mxcsr",   "i386_mxcsr", FltSave.MxCsr),
};

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
    memory_disasm_one_x86_insn,
    be_x86_64_insert_Xpoint,
    be_x86_64_remove_Xpoint,
    be_x86_64_is_watchpoint_set,
    be_x86_64_clear_watchpoint,
    be_x86_64_adjust_pc_for_break,
    be_x86_64_get_context,
    be_x86_64_set_context,
    be_x86_64_gdb_register_map,
    ARRAY_SIZE(be_x86_64_gdb_register_map),
};
#endif
