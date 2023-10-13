/*
 * Debugger i386 specific functions
 *
 * Copyright 2004 Eric Pouech
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

#if defined(__i386__) || defined(__x86_64__)

WINE_DEFAULT_DEBUG_CHANNEL(winedbg);

#define STEP_FLAG 0x00000100 /* single step flag */
#define V86_FLAG  0x00020000

#define IS_VM86_MODE(ctx) (ctx->EFlags & V86_FLAG)

static const unsigned first_ldt_entry = 32;

static ADDRESS_MODE get_selector_type(HANDLE hThread, const WOW64_CONTEXT *ctx, WORD sel)
{
    LDT_ENTRY	le;

    if (IS_VM86_MODE(ctx)) return AddrModeReal;
    /* null or system selector */
    if (!(sel & 4) || ((sel >> 3) < first_ldt_entry)) return AddrModeFlat;
    if (dbg_curr_process->process_io->get_selector(hThread, sel, &le))
    {
        ULONG base = (le.HighWord.Bits.BaseHi << 24) + (le.HighWord.Bits.BaseMid << 16) + le.BaseLow;
        if (le.HighWord.Bits.Default_Big)
            return base == 0 ? AddrModeFlat : AddrMode1632;
        return AddrMode1616;
    }
    /* selector doesn't exist */
    return -1;
}

static void* be_i386_linearize(HANDLE hThread, const ADDRESS64* addr)
{
    LDT_ENTRY	le;

    switch (addr->Mode)
    {
    case AddrModeReal:
        return (void*)((DWORD_PTR)(LOWORD(addr->Segment) << 4) + (DWORD_PTR)addr->Offset);
    case AddrMode1632:
        if (!(addr->Segment & 4) || ((addr->Segment >> 3) < first_ldt_entry))
            return (void*)(DWORD_PTR)addr->Offset;
        /* fall through */
    case AddrMode1616:
        if (!dbg_curr_process->process_io->get_selector(hThread, addr->Segment, &le)) return NULL;
        return (void*)((le.HighWord.Bits.BaseHi << 24) + 
                       (le.HighWord.Bits.BaseMid << 16) + le.BaseLow +
                       (DWORD_PTR)addr->Offset);
    case AddrModeFlat:
        return (void*)(DWORD_PTR)addr->Offset;
    }
    return NULL;
}

static BOOL be_i386_build_addr(HANDLE hThread, const dbg_ctx_t *ctx, ADDRESS64* addr,
                               unsigned seg, DWORD64 offset)
{
    addr->Mode    = AddrModeFlat;
    addr->Segment = seg;
    addr->Offset  = offset;
    if (seg)
    {
        addr->Mode = get_selector_type(hThread, &ctx->x86, seg);
        switch (addr->Mode)
        {
        case AddrModeReal:
        case AddrMode1616:
            addr->Offset &= 0xffff;
            break;
        case AddrModeFlat:
        case AddrMode1632:
            break;
        default:
            addr->Mode = -1;
            return FALSE;
        }            
    }
    return TRUE;
}

static BOOL be_i386_get_addr(HANDLE hThread, const dbg_ctx_t *ctx,
                             enum be_cpu_addr bca, ADDRESS64* addr)
{
    switch (bca)
    {
    case be_cpu_addr_pc:
        return be_i386_build_addr(hThread, ctx, addr, ctx->x86.SegCs, ctx->x86.Eip);
    case be_cpu_addr_stack:
        return be_i386_build_addr(hThread, ctx, addr, ctx->x86.SegSs, ctx->x86.Esp);
    case be_cpu_addr_frame:
        return be_i386_build_addr(hThread, ctx, addr, ctx->x86.SegSs, ctx->x86.Ebp);
    }
    return FALSE;
}

static BOOL be_i386_get_register_info(int regno, enum be_cpu_addr* kind)
{
    switch (regno)
    {
    case CV_REG_EIP: *kind = be_cpu_addr_pc; return TRUE;
    case CV_REG_EBP: *kind = be_cpu_addr_frame; return TRUE;
    case CV_REG_ESP: *kind = be_cpu_addr_stack; return TRUE;
    }
    return FALSE;
}

static void be_i386_single_step(dbg_ctx_t *ctx, BOOL enable)
{
    if (enable) ctx->x86.EFlags |= STEP_FLAG;
    else ctx->x86.EFlags &= ~STEP_FLAG;
}

static void be_i386_all_print_context(HANDLE hThread, const dbg_ctx_t *pctx)
{
    static const char mxcsr_flags[16][4] = { "IE", "DE", "ZE", "OE", "UE", "PE", "DAZ", "IM",
                                             "DM", "ZM", "OM", "UM", "PM", "R-", "R+", "FZ" };
    const WOW64_CONTEXT *ctx = &pctx->x86;
    XSAVE_FORMAT *xmm_area;
    int         cnt;

    /* Break out the FPU state and the floating point registers    */
    dbg_printf("Floating Point Unit status:\n");
    dbg_printf(" FLCW:%04x ", LOWORD(ctx->FloatSave.ControlWord));
    dbg_printf(" FLTW:%04x ", LOWORD(ctx->FloatSave.TagWord));
    dbg_printf(" FLEO:%08x ", (unsigned int) ctx->FloatSave.ErrorOffset);
    dbg_printf(" FLSW:%04x", LOWORD(ctx->FloatSave.StatusWord));

    /* Isolate the condition code bits - note they are not contiguous */
    dbg_printf("(CC:%ld%ld%ld%ld", (ctx->FloatSave.StatusWord & 0x00004000) >> 14,
               (ctx->FloatSave.StatusWord & 0x00000400) >> 10,
               (ctx->FloatSave.StatusWord & 0x00000200) >> 9,
               (ctx->FloatSave.StatusWord & 0x00000100) >> 8);

    /* Now pull out the 3 bit of the TOP stack pointer */
    dbg_printf(" TOP:%01x", (unsigned int) (ctx->FloatSave.StatusWord & 0x00003800) >> 11);

    /* Lets analyse the error bits and indicate the status  
     * the Invalid Op flag has sub status which is tested as follows */
    if (ctx->FloatSave.StatusWord & 0x00000001) {     /* Invalid Fl OP   */
       if (ctx->FloatSave.StatusWord & 0x00000040) {  /* Stack Fault     */
          if (ctx->FloatSave.StatusWord & 0x00000200) /* C1 says Overflow */
             dbg_printf(" #IE(Stack Overflow)");      
          else
             dbg_printf(" #IE(Stack Underflow)");     /* Underflow */
       }
       else  dbg_printf(" #IE(Arthimetic error)");    /* Invalid Fl OP   */
    }

    if (ctx->FloatSave.StatusWord & 0x00000002) dbg_printf(" #DE"); /* Denormalised OP */
    if (ctx->FloatSave.StatusWord & 0x00000004) dbg_printf(" #ZE"); /* Zero Divide     */
    if (ctx->FloatSave.StatusWord & 0x00000008) dbg_printf(" #OE"); /* Overflow        */
    if (ctx->FloatSave.StatusWord & 0x00000010) dbg_printf(" #UE"); /* Underflow       */
    if (ctx->FloatSave.StatusWord & 0x00000020) dbg_printf(" #PE"); /* Precision error */
    if (ctx->FloatSave.StatusWord & 0x00000040)
       if (!(ctx->FloatSave.StatusWord & 0x00000001))
           dbg_printf(" #SE");                 /* Stack Fault (don't think this can occur) */
    if (ctx->FloatSave.StatusWord & 0x00000080) dbg_printf(" #ES"); /* Error Summary   */
    if (ctx->FloatSave.StatusWord & 0x00008000) dbg_printf(" #FB"); /* FPU Busy        */
    dbg_printf(")\n");
    
    /* Here are the rest of the registers */
    dbg_printf(" FLES:%08lx  FLDO:%08lx  FLDS:%08lx  FLCNS:%08lx\n",
               ctx->FloatSave.ErrorSelector,
               ctx->FloatSave.DataOffset,
               ctx->FloatSave.DataSelector,
               ctx->FloatSave.Cr0NpxState);

    /* Now for the floating point registers */
    dbg_printf("Floating Point Registers:\n");
    for (cnt = 0; cnt < 8; cnt++)
    {
        const BYTE *p = &ctx->FloatSave.RegisterArea[cnt * 10];
        if (cnt == 4) dbg_printf("\n");
        dbg_printf(" ST%d:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x ", cnt,
                   p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9] );
    }

    xmm_area = (XSAVE_FORMAT *) &ctx->ExtendedRegisters;

    dbg_printf(" mxcsr: %04lx (", xmm_area->MxCsr );
    for (cnt = 0; cnt < 16; cnt++)
        if (xmm_area->MxCsr & (1 << cnt)) dbg_printf( " %s", mxcsr_flags[cnt] );
    dbg_printf(" )\n");

    for (cnt = 0; cnt < 8; cnt++)
    {
        dbg_printf( " xmm%u: uint=%08x%08x%08x%08x", cnt,
                    *((unsigned int *)&xmm_area->XmmRegisters[cnt] + 3),
                    *((unsigned int *)&xmm_area->XmmRegisters[cnt] + 2),
                    *((unsigned int *)&xmm_area->XmmRegisters[cnt] + 1),
                    *((unsigned int *)&xmm_area->XmmRegisters[cnt] + 0));
        dbg_printf( " double={%g; %g}", *(double *)&xmm_area->XmmRegisters[cnt].Low,
                    *(double *)&xmm_area->XmmRegisters[cnt].High );
        dbg_printf( " float={%g; %g; %g; %g}\n",
                    (double)*((float *)&xmm_area->XmmRegisters[cnt] + 0),
                    (double)*((float *)&xmm_area->XmmRegisters[cnt] + 1),
                    (double)*((float *)&xmm_area->XmmRegisters[cnt] + 2),
                    (double)*((float *)&xmm_area->XmmRegisters[cnt] + 3) );
    }
    dbg_printf("\n");
}

static void be_i386_print_context(HANDLE hThread, const dbg_ctx_t *pctx, int all_regs)
{
    static const char flags[] = "aVR-N--ODITSZ-A-P-C";
    const WOW64_CONTEXT *ctx = &pctx->x86;
    int i;
    char        buf[33];

    dbg_printf("Register dump:\n");

    /* First get the segment registers out of the way */
    dbg_printf(" CS:%04x SS:%04x DS:%04x ES:%04x FS:%04x GS:%04x",
               (WORD)ctx->SegCs, (WORD)ctx->SegSs,
               (WORD)ctx->SegDs, (WORD)ctx->SegEs,
               (WORD)ctx->SegFs, (WORD)ctx->SegGs);

    strcpy(buf, flags);
    for (i = 0; buf[i]; i++)
        if (buf[i] != '-' && !(ctx->EFlags & (1 << (sizeof(flags) - 2 - i))))
            buf[i] = ' ';

    switch (get_selector_type(hThread, ctx, ctx->SegCs))
    {
    case AddrMode1616:
    case AddrModeReal:
        dbg_printf("\n IP:%04x SP:%04x BP:%04x FLAGS:%04x(%s)\n",
                   LOWORD(ctx->Eip), LOWORD(ctx->Esp),
                   LOWORD(ctx->Ebp), LOWORD(ctx->EFlags), buf);
	dbg_printf(" AX:%04x BX:%04x CX:%04x DX:%04x SI:%04x DI:%04x\n",
                   LOWORD(ctx->Eax), LOWORD(ctx->Ebx),
                   LOWORD(ctx->Ecx), LOWORD(ctx->Edx),
                   LOWORD(ctx->Esi), LOWORD(ctx->Edi));
        break;
    case AddrModeFlat:
    case AddrMode1632:
        dbg_printf("\n EIP:%08lx ESP:%08lx EBP:%08lx EFLAGS:%08lx(%s)\n",
                   ctx->Eip, ctx->Esp, ctx->Ebp, ctx->EFlags, buf);
        dbg_printf(" EAX:%08lx EBX:%08lx ECX:%08lx EDX:%08lx\n",
                   ctx->Eax, ctx->Ebx, ctx->Ecx, ctx->Edx);
        dbg_printf(" ESI:%08lx EDI:%08lx\n",
                   ctx->Esi, ctx->Edi);
        break;
    }

    if (all_regs) be_i386_all_print_context(hThread, pctx);

}

static void be_i386_print_segment_info(HANDLE hThread, const dbg_ctx_t *ctx)
{
    if (get_selector_type(hThread, &ctx->x86, ctx->x86.SegCs) == AddrMode1616)
    {
        info_win32_segments(ctx->x86.SegDs >> 3, 1);
        if (ctx->x86.SegEs != ctx->x86.SegDs)
            info_win32_segments(ctx->x86.SegEs >> 3, 1);
    }
    info_win32_segments(ctx->x86.SegFs >> 3, 1);
}

static struct dbg_internal_var be_i386_ctx[] =
{
    {CV_REG_AL,         "AL",           (void*)FIELD_OFFSET(WOW64_CONTEXT, Eax),     dbg_itype_unsigned_int8},
    {CV_REG_CL,         "CL",           (void*)FIELD_OFFSET(WOW64_CONTEXT, Ecx),     dbg_itype_unsigned_int8},
    {CV_REG_DL,         "DL",           (void*)FIELD_OFFSET(WOW64_CONTEXT, Edx),     dbg_itype_unsigned_int8},
    {CV_REG_BL,         "BL",           (void*)FIELD_OFFSET(WOW64_CONTEXT, Ebx),     dbg_itype_unsigned_int8},
    {CV_REG_AH,         "AH",           (void*)(FIELD_OFFSET(WOW64_CONTEXT, Eax)+1), dbg_itype_unsigned_int8},
    {CV_REG_CH,         "CH",           (void*)(FIELD_OFFSET(WOW64_CONTEXT, Ecx)+1), dbg_itype_unsigned_int8},
    {CV_REG_DH,         "DH",           (void*)(FIELD_OFFSET(WOW64_CONTEXT, Edx)+1), dbg_itype_unsigned_int8},
    {CV_REG_BH,         "BH",           (void*)(FIELD_OFFSET(WOW64_CONTEXT, Ebx)+1), dbg_itype_unsigned_int8},
    {CV_REG_AX,         "AX",           (void*)FIELD_OFFSET(WOW64_CONTEXT, Eax),     dbg_itype_unsigned_int16},
    {CV_REG_CX,         "CX",           (void*)FIELD_OFFSET(WOW64_CONTEXT, Ecx),     dbg_itype_unsigned_int16},
    {CV_REG_DX,         "DX",           (void*)FIELD_OFFSET(WOW64_CONTEXT, Edx),     dbg_itype_unsigned_int16},
    {CV_REG_BX,         "BX",           (void*)FIELD_OFFSET(WOW64_CONTEXT, Ebx),     dbg_itype_unsigned_int16},
    {CV_REG_SP,         "SP",           (void*)FIELD_OFFSET(WOW64_CONTEXT, Esp),     dbg_itype_unsigned_int16},
    {CV_REG_BP,         "BP",           (void*)FIELD_OFFSET(WOW64_CONTEXT, Ebp),     dbg_itype_unsigned_int16},
    {CV_REG_SI,         "SI",           (void*)FIELD_OFFSET(WOW64_CONTEXT, Esi),     dbg_itype_unsigned_int16},
    {CV_REG_DI,         "DI",           (void*)FIELD_OFFSET(WOW64_CONTEXT, Edi),     dbg_itype_unsigned_int16},
    {CV_REG_EAX,        "EAX",          (void*)FIELD_OFFSET(WOW64_CONTEXT, Eax),     dbg_itype_unsigned_int32},
    {CV_REG_ECX,        "ECX",          (void*)FIELD_OFFSET(WOW64_CONTEXT, Ecx),     dbg_itype_unsigned_int32},
    {CV_REG_EDX,        "EDX",          (void*)FIELD_OFFSET(WOW64_CONTEXT, Edx),     dbg_itype_unsigned_int32},
    {CV_REG_EBX,        "EBX",          (void*)FIELD_OFFSET(WOW64_CONTEXT, Ebx),     dbg_itype_unsigned_int32},
    {CV_REG_ESP,        "ESP",          (void*)FIELD_OFFSET(WOW64_CONTEXT, Esp),     dbg_itype_unsigned_int32},
    {CV_REG_EBP,        "EBP",          (void*)FIELD_OFFSET(WOW64_CONTEXT, Ebp),     dbg_itype_unsigned_int32},
    {CV_REG_ESI,        "ESI",          (void*)FIELD_OFFSET(WOW64_CONTEXT, Esi),     dbg_itype_unsigned_int32},
    {CV_REG_EDI,        "EDI",          (void*)FIELD_OFFSET(WOW64_CONTEXT, Edi),     dbg_itype_unsigned_int32},
    {CV_REG_ES,         "ES",           (void*)FIELD_OFFSET(WOW64_CONTEXT, SegEs),   dbg_itype_unsigned_int16},
    {CV_REG_CS,         "CS",           (void*)FIELD_OFFSET(WOW64_CONTEXT, SegCs),   dbg_itype_unsigned_int16},
    {CV_REG_SS,         "SS",           (void*)FIELD_OFFSET(WOW64_CONTEXT, SegSs),   dbg_itype_unsigned_int16},
    {CV_REG_DS,         "DS",           (void*)FIELD_OFFSET(WOW64_CONTEXT, SegDs),   dbg_itype_unsigned_int16},
    {CV_REG_FS,         "FS",           (void*)FIELD_OFFSET(WOW64_CONTEXT, SegFs),   dbg_itype_unsigned_int16},
    {CV_REG_GS,         "GS",           (void*)FIELD_OFFSET(WOW64_CONTEXT, SegGs),   dbg_itype_unsigned_int16},
    {CV_REG_IP,         "IP",           (void*)FIELD_OFFSET(WOW64_CONTEXT, Eip),     dbg_itype_unsigned_int16},
    {CV_REG_FLAGS,      "FLAGS",        (void*)FIELD_OFFSET(WOW64_CONTEXT, EFlags),  dbg_itype_unsigned_int16},
    {CV_REG_EIP,        "EIP",          (void*)FIELD_OFFSET(WOW64_CONTEXT, Eip),     dbg_itype_unsigned_int32},
    {CV_REG_EFLAGS,     "EFLAGS",       (void*)FIELD_OFFSET(WOW64_CONTEXT, EFlags),  dbg_itype_unsigned_int32},
    {CV_REG_ST0,        "ST0",          (void*)FIELD_OFFSET(WOW64_CONTEXT, FloatSave.RegisterArea[ 0]), dbg_itype_long_real},
    {CV_REG_ST0+1,      "ST1",          (void*)FIELD_OFFSET(WOW64_CONTEXT, FloatSave.RegisterArea[10]), dbg_itype_long_real},
    {CV_REG_ST0+2,      "ST2",          (void*)FIELD_OFFSET(WOW64_CONTEXT, FloatSave.RegisterArea[20]), dbg_itype_long_real},
    {CV_REG_ST0+3,      "ST3",          (void*)FIELD_OFFSET(WOW64_CONTEXT, FloatSave.RegisterArea[30]), dbg_itype_long_real},
    {CV_REG_ST0+4,      "ST4",          (void*)FIELD_OFFSET(WOW64_CONTEXT, FloatSave.RegisterArea[40]), dbg_itype_long_real},
    {CV_REG_ST0+5,      "ST5",          (void*)FIELD_OFFSET(WOW64_CONTEXT, FloatSave.RegisterArea[50]), dbg_itype_long_real},
    {CV_REG_ST0+6,      "ST6",          (void*)FIELD_OFFSET(WOW64_CONTEXT, FloatSave.RegisterArea[60]), dbg_itype_long_real},
    {CV_REG_ST0+7,      "ST7",          (void*)FIELD_OFFSET(WOW64_CONTEXT, FloatSave.RegisterArea[70]), dbg_itype_long_real},
    {CV_AMD64_XMM0,     "XMM0",         (void*)(FIELD_OFFSET(WOW64_CONTEXT, ExtendedRegisters) + FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters[0])), dbg_itype_m128a},
    {CV_AMD64_XMM0+1,   "XMM1",         (void*)(FIELD_OFFSET(WOW64_CONTEXT, ExtendedRegisters) + FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters[1])), dbg_itype_m128a},
    {CV_AMD64_XMM0+2,   "XMM2",         (void*)(FIELD_OFFSET(WOW64_CONTEXT, ExtendedRegisters) + FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters[2])), dbg_itype_m128a},
    {CV_AMD64_XMM0+3,   "XMM3",         (void*)(FIELD_OFFSET(WOW64_CONTEXT, ExtendedRegisters) + FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters[3])), dbg_itype_m128a},
    {CV_AMD64_XMM0+4,   "XMM4",         (void*)(FIELD_OFFSET(WOW64_CONTEXT, ExtendedRegisters) + FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters[4])), dbg_itype_m128a},
    {CV_AMD64_XMM0+5,   "XMM5",         (void*)(FIELD_OFFSET(WOW64_CONTEXT, ExtendedRegisters) + FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters[5])), dbg_itype_m128a},
    {CV_AMD64_XMM0+6,   "XMM6",         (void*)(FIELD_OFFSET(WOW64_CONTEXT, ExtendedRegisters) + FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters[6])), dbg_itype_m128a},
    {CV_AMD64_XMM0+7,   "XMM7",         (void*)(FIELD_OFFSET(WOW64_CONTEXT, ExtendedRegisters) + FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters[7])), dbg_itype_m128a},
    {0,                 NULL,           0,                                           dbg_itype_none}
};

static BOOL be_i386_is_step_over_insn(const void* insn)
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

static BOOL be_i386_is_function_return(const void* insn)
{
    BYTE ch;

    if (!dbg_read_memory(insn, &ch, sizeof(ch))) return FALSE;
    if (ch == 0xF3) /* REP */
    {
        insn = (const char*)insn + 1;
        if (!dbg_read_memory(insn, &ch, sizeof(ch))) return FALSE;
    }
    return (ch == 0xC2) || (ch == 0xC3);
}

static BOOL be_i386_is_break_insn(const void* insn)
{
    BYTE        c;

    if (!dbg_read_memory(insn, &c, sizeof(c))) return FALSE;
    return c == 0xCC;
}

static unsigned get_size(ADDRESS_MODE am)
{
    if (am == AddrModeReal || am == AddrMode1616) return 16;
    return 32;
}

static BOOL fetch_value(const char* addr, unsigned sz, int* value)
{
    char        value8;
    short       value16;

    switch (sz)
    {
    case 8:
        if (!dbg_read_memory(addr, &value8, sizeof(value8)))
            return FALSE;
        *value = value8;
        break;
    case 16:
        if (!dbg_read_memory(addr, &value16, sizeof(value16)))
            return FALSE;
        *value = value16;
        break;
    case 32:
        if (!dbg_read_memory(addr, value, sizeof(*value)))
            return FALSE;
        break;
    default: return FALSE;
    }
    return TRUE;
}

static BOOL be_i386_is_func_call(const void* insn, ADDRESS64* callee)
{
    BYTE                ch;
    int                 delta;
    short               segment;
    unsigned            dst = 0;
    unsigned            operand_size;
    ADDRESS_MODE        cs_addr_mode;

    cs_addr_mode = get_selector_type(dbg_curr_thread->handle, &dbg_context.x86,
                                     dbg_context.x86.SegCs);
    operand_size = get_size(cs_addr_mode);

    /* get operand_size (also getting rid of the various prefixes */
    do
    {
        if (!dbg_read_memory(insn, &ch, sizeof(ch))) return FALSE;
        if (ch == 0x66)
        {
            operand_size = 48 - operand_size; /* 16 => 32, 32 => 16 */
            insn = (const char*)insn + 1;
        }
    } while (ch == 0x66 || ch == 0x67);

    switch (ch)
    {
    case 0xe8: /* relative near call */
        callee->Mode = cs_addr_mode;
        if (!fetch_value((const char*)insn + 1, operand_size, &delta))
            return FALSE;
        callee->Segment = dbg_context.x86.SegCs;
        callee->Offset = (DWORD_PTR)insn + 1 + (operand_size / 8) + delta;
        return TRUE;

    case 0x9a: /* absolute far call */
        if (!dbg_read_memory((const char*)insn + 1 + operand_size / 8,
                             &segment, sizeof(segment)))
            return FALSE;
        callee->Mode = get_selector_type(dbg_curr_thread->handle, &dbg_context.x86,
                                         segment);
        if (!fetch_value((const char*)insn + 1, operand_size, &delta))
            return FALSE;
        callee->Segment = segment;
        callee->Offset = delta;
        return TRUE;

    case 0xff:
        if (!dbg_read_memory((const char*)insn + 1, &ch, sizeof(ch)))
            return FALSE;
        /* keep only the CALL and LCALL insn:s */
        switch ((ch >> 3) & 0x07)
        {
        case 0x02:
            segment = dbg_context.x86.SegCs;
            break;
        case 0x03:
            if (!dbg_read_memory((const char*)insn + 1 + operand_size / 8,
                                 &segment, sizeof(segment)))
                return FALSE;
            break;
        default: return FALSE;
        }
        /* FIXME: we only support the 32 bit far calls for now */
        if (operand_size != 32)
        {
            WINE_FIXME("Unsupported yet call insn (0xFF 0x%02x) with 16 bit operand-size at %p\n", ch, insn);
            return FALSE;
        }
        switch (ch & 0xC7) /* keep Mod R/M only (skip reg) */
        {
        case 0x04:
        case 0x44:
        case 0x84:
            WINE_FIXME("Unsupported yet call insn (0xFF 0x%02x) (SIB bytes) at %p\n", ch, insn);
            return FALSE;
        case 0x05: /* addr32 */
            if ((ch & 0x38) == 0x10 || /* call */
                (ch & 0x38) == 0x18)   /* lcall */
            {
                void *addr;
                if (!dbg_read_memory((const char *)insn + 2, &addr, sizeof(addr)))
                    return FALSE;
                if ((ch & 0x38) == 0x18)   /* lcall */
                {
                    if (!dbg_read_memory((const char*)addr + operand_size, &segment, sizeof(segment)))
                        return FALSE;
                }
                else segment = dbg_context.x86.SegCs;
                if (!dbg_read_memory((const char*)addr, &dst, sizeof(dst)))
                    return FALSE;
                callee->Mode = get_selector_type(dbg_curr_thread->handle, &dbg_context.x86, segment);
                callee->Segment = segment;
                callee->Offset = dst;
                return TRUE;
            }
            return FALSE;
        default:
            switch (ch & 0x07)
            {
            case 0x00: dst = dbg_context.x86.Eax; break;
            case 0x01: dst = dbg_context.x86.Ecx; break;
            case 0x02: dst = dbg_context.x86.Edx; break;
            case 0x03: dst = dbg_context.x86.Ebx; break;
            case 0x04: dst = dbg_context.x86.Esp; break;
            case 0x05: dst = dbg_context.x86.Ebp; break;
            case 0x06: dst = dbg_context.x86.Esi; break;
            case 0x07: dst = dbg_context.x86.Edi; break;
            }
            if ((ch >> 6) != 0x03) /* indirect address */
            {
                if (ch >> 6) /* we got a displacement */
                {
                    if (!fetch_value((const char*)insn + 2, (ch >> 6) == 0x01 ? 8 : 32, &delta))
                        return FALSE;
                    dst += delta;
                }
                if (((ch >> 3) & 0x07) == 0x03) /* LCALL */
                {
                    if (!dbg_read_memory((const char*)(UINT_PTR)dst + operand_size, &segment, sizeof(segment)))
                        return FALSE;
                }
                else segment = dbg_context.x86.SegCs;
                if (!dbg_read_memory((const char*)(UINT_PTR)dst, &delta, sizeof(delta)))
                    return FALSE;
                callee->Mode = get_selector_type(dbg_curr_thread->handle, &dbg_context.x86,
                                                 segment);
                callee->Segment = segment;
                callee->Offset = delta;
            }
            else
            {
                callee->Mode = cs_addr_mode;
                callee->Segment = dbg_context.x86.SegCs;
                callee->Offset = dst;
            }
        }
        return TRUE;

    default:
        return FALSE;
    }
}

static BOOL be_i386_is_jump(const void* insn, ADDRESS64* jumpee)
{
    BYTE                ch;
    int                 delta;
    unsigned            operand_size;
    ADDRESS_MODE        cs_addr_mode;

    cs_addr_mode = get_selector_type(dbg_curr_thread->handle, &dbg_context.x86,
                                     dbg_context.x86.SegCs);
    operand_size = get_size(cs_addr_mode);

    /* get operand_size (also getting rid of the various prefixes */
    do
    {
        if (!dbg_read_memory(insn, &ch, sizeof(ch))) return FALSE;
        if (ch == 0x66)
        {
            operand_size = 48 - operand_size; /* 16 => 32, 32 => 16 */
            insn = (const char*)insn + 1;
        }
    } while (ch == 0x66 || ch == 0x67);

    switch (ch)
    {
    case 0xe9: /* jmp near */
        jumpee->Mode = cs_addr_mode;
        if (!fetch_value((const char*)insn + 1, operand_size, &delta))
            return FALSE;
        jumpee->Segment = dbg_context.x86.SegCs;
        jumpee->Offset = (DWORD_PTR)insn + 1 + (operand_size / 8) + delta;
        return TRUE;
    default: WINE_FIXME("unknown %x\n", ch); return FALSE;
    }
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

static inline int be_i386_get_unused_DR(dbg_ctx_t *pctx, DWORD** r)
{
    WOW64_CONTEXT *ctx = &pctx->x86;

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

static BOOL be_i386_insert_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                  dbg_ctx_t *ctx, enum be_xpoint_type type,
                                  void* addr, unsigned *val, unsigned size)
{
    unsigned char       ch;
    SIZE_T              sz;
    DWORD              *pr;
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
        if ((reg = be_i386_get_unused_DR(ctx, &pr)) == -1) return FALSE;
        *pr = (DWORD_PTR)addr;
        if (type != be_xpoint_watch_exec) switch (size)
        {
        case 4: bits |= DR7_LEN_4; break;
        case 2: bits |= DR7_LEN_2; break;
        case 1: bits |= DR7_LEN_1; break;
        default: return FALSE;
        }
        *val = reg;
        /* clear old values */
        ctx->x86.Dr7 &= ~(0x0F << (DR7_CONTROL_SHIFT + DR7_CONTROL_SIZE * reg));
        /* set the correct ones */
        ctx->x86.Dr7 |= bits << (DR7_CONTROL_SHIFT + DR7_CONTROL_SIZE * reg);
        ctx->x86.Dr7 |= DR7_ENABLE_MASK(reg) | DR7_LOCAL_SLOWDOWN;
        break;
    default:
        dbg_printf("Unknown bp type %c\n", type);
        return FALSE;
    }
    return TRUE;
}

static BOOL be_i386_remove_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
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
        ctx->x86.Dr7 &= ~DR7_ENABLE_MASK(val);
        break;
    default:
        dbg_printf("Unknown bp type %c\n", type);
        return FALSE;
    }
    return TRUE;
}

static BOOL be_i386_is_watchpoint_set(const dbg_ctx_t *ctx, unsigned idx)
{
    return ctx->x86.Dr6 & (1 << idx);
}

static void be_i386_clear_watchpoint(dbg_ctx_t *ctx, unsigned idx)
{
    ctx->x86.Dr6 &= ~(1 << idx);
}

static int be_i386_adjust_pc_for_break(dbg_ctx_t *ctx, BOOL way)
{
    if (way)
    {
        ctx->x86.Eip--;
        return -1;
    }
    ctx->x86.Eip++;
    return 1;
}

static BOOL be_i386_get_context(HANDLE thread, dbg_ctx_t *ctx)
{
    ctx->x86.ContextFlags = WOW64_CONTEXT_ALL;
    return Wow64GetThreadContext(thread, &ctx->x86);
}

static BOOL be_i386_set_context(HANDLE thread, const dbg_ctx_t *ctx)
{
    return Wow64SetThreadContext(thread, &ctx->x86);
}

#define REG(f,n,t,r)  {f, n, t, FIELD_OFFSET(WOW64_CONTEXT, r), sizeof(((WOW64_CONTEXT*)NULL)->r)}

static struct gdb_register be_i386_gdb_register_map[] = {
    REG("core", "eax",    NULL,          Eax),
    REG(NULL,   "ecx",    NULL,          Ecx),
    REG(NULL,   "edx",    NULL,          Edx),
    REG(NULL,   "ebx",    NULL,          Ebx),
    REG(NULL,   "esp",    "data_ptr",    Esp),
    REG(NULL,   "ebp",    "data_ptr",    Ebp),
    REG(NULL,   "esi",    NULL,          Esi),
    REG(NULL,   "edi",    NULL,          Edi),
    REG(NULL,   "eip",    "code_ptr",    Eip),
    REG(NULL,   "eflags", "i386_eflags", EFlags),
    REG(NULL,   "cs",     NULL,          SegCs),
    REG(NULL,   "ss",     NULL,          SegSs),
    REG(NULL,   "ds",     NULL,          SegDs),
    REG(NULL,   "es",     NULL,          SegEs),
    REG(NULL,   "fs",     NULL,          SegFs),
    REG(NULL,   "gs",     NULL,          SegGs),
    { NULL,     "st0",    "i387_ext",    FIELD_OFFSET(WOW64_CONTEXT, FloatSave.RegisterArea[ 0]), 10},
    { NULL,     "st1",    "i387_ext",    FIELD_OFFSET(WOW64_CONTEXT, FloatSave.RegisterArea[10]), 10},
    { NULL,     "st2",    "i387_ext",    FIELD_OFFSET(WOW64_CONTEXT, FloatSave.RegisterArea[20]), 10},
    { NULL,     "st3",    "i387_ext",    FIELD_OFFSET(WOW64_CONTEXT, FloatSave.RegisterArea[30]), 10},
    { NULL,     "st4",    "i387_ext",    FIELD_OFFSET(WOW64_CONTEXT, FloatSave.RegisterArea[40]), 10},
    { NULL,     "st5",    "i387_ext",    FIELD_OFFSET(WOW64_CONTEXT, FloatSave.RegisterArea[50]), 10},
    { NULL,     "st6",    "i387_ext",    FIELD_OFFSET(WOW64_CONTEXT, FloatSave.RegisterArea[60]), 10},
    { NULL,     "st7",    "i387_ext",    FIELD_OFFSET(WOW64_CONTEXT, FloatSave.RegisterArea[70]), 10},
    { NULL,     "fctrl",  NULL,          FIELD_OFFSET(WOW64_CONTEXT, FloatSave.ControlWord), 2},
    { NULL,     "fstat",  NULL,          FIELD_OFFSET(WOW64_CONTEXT, FloatSave.StatusWord), 2},
    { NULL,     "ftag",   NULL,          FIELD_OFFSET(WOW64_CONTEXT, FloatSave.TagWord), 2},
    { NULL,     "fiseg",  NULL,          FIELD_OFFSET(WOW64_CONTEXT, FloatSave.ErrorSelector), 2},
    REG(NULL,   "fioff",  NULL,          FloatSave.ErrorOffset),
    { NULL,     "foseg",  NULL,          FIELD_OFFSET(WOW64_CONTEXT, FloatSave.DataSelector), 2},
    REG(NULL,   "fooff",  NULL,          FloatSave.DataOffset),
    { NULL,     "fop",    NULL,          FIELD_OFFSET(WOW64_CONTEXT, FloatSave.ErrorSelector)+2, 2},

    { "sse", "xmm0",  "vec128",     FIELD_OFFSET(WOW64_CONTEXT, ExtendedRegisters) + FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters[0]), 16},
    { NULL,  "xmm1",  "vec128",     FIELD_OFFSET(WOW64_CONTEXT, ExtendedRegisters) + FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters[1]), 16},
    { NULL,  "xmm2",  "vec128",     FIELD_OFFSET(WOW64_CONTEXT, ExtendedRegisters) + FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters[2]), 16},
    { NULL,  "xmm3",  "vec128",     FIELD_OFFSET(WOW64_CONTEXT, ExtendedRegisters) + FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters[3]), 16},
    { NULL,  "xmm4",  "vec128",     FIELD_OFFSET(WOW64_CONTEXT, ExtendedRegisters) + FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters[4]), 16},
    { NULL,  "xmm5",  "vec128",     FIELD_OFFSET(WOW64_CONTEXT, ExtendedRegisters) + FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters[5]), 16},
    { NULL,  "xmm6",  "vec128",     FIELD_OFFSET(WOW64_CONTEXT, ExtendedRegisters) + FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters[6]), 16},
    { NULL,  "xmm7",  "vec128",     FIELD_OFFSET(WOW64_CONTEXT, ExtendedRegisters) + FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters[7]), 16},
    { NULL,  "mxcsr", "i386_mxcsr", FIELD_OFFSET(WOW64_CONTEXT, ExtendedRegisters) + FIELD_OFFSET(XSAVE_FORMAT, MxCsr), 4},
};

struct backend_cpu be_i386 =
{
    IMAGE_FILE_MACHINE_I386,
    4,
    be_i386_linearize,
    be_i386_build_addr,
    be_i386_get_addr,
    be_i386_get_register_info,
    be_i386_single_step,
    be_i386_print_context,
    be_i386_print_segment_info,
    be_i386_ctx,
    be_i386_is_step_over_insn,
    be_i386_is_function_return,
    be_i386_is_break_insn,
    be_i386_is_func_call,
    be_i386_is_jump,
    memory_disasm_one_x86_insn,
    be_i386_insert_Xpoint,
    be_i386_remove_Xpoint,
    be_i386_is_watchpoint_set,
    be_i386_clear_watchpoint,
    be_i386_adjust_pc_for_break,
    be_i386_get_context,
    be_i386_set_context,
    be_i386_gdb_register_map,
    ARRAY_SIZE(be_i386_gdb_register_map),
};
#endif
