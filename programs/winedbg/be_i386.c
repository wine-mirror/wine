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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "debugger.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winedbg);

#ifdef __i386__

  /* debugger/db_disasm.c */
extern void             be_i386_disasm_one_insn(ADDRESS* addr, int display);

#define STEP_FLAG 0x00000100 /* single step flag */
#define V86_FLAG  0x00020000

#define IS_VM86_MODE(ctx) (ctx->EFlags & V86_FLAG)

static ADDRESS_MODE get_selector_type(HANDLE hThread, const CONTEXT* ctx, WORD sel)
{
    LDT_ENTRY	le;

    if (IS_VM86_MODE(ctx)) return AddrModeReal;
    /* null or system selector */
    if (!(sel & 4) || ((sel >> 3) < 17)) return AddrModeFlat;
    if (GetThreadSelectorEntry(hThread, sel, &le))
        return le.HighWord.Bits.Default_Big ? AddrMode1632 : AddrMode1616;
    /* selector doesn't exist */
    return -1;
}

static void* be_i386_linearize(HANDLE hThread, const ADDRESS* addr)
{
    LDT_ENTRY	le;

    switch (addr->Mode)
    {
    case AddrModeReal:
        return (void*)((DWORD)(LOWORD(addr->Segment) << 4) + addr->Offset);
    case AddrMode1632:
        if (!(addr->Segment & 4) || ((addr->Segment >> 3) < 17))
            return (void*)addr->Offset;
        /* fall through */
    case AddrMode1616:
        if (!GetThreadSelectorEntry(hThread, addr->Segment, &le)) return NULL;
        return (void*)((le.HighWord.Bits.BaseHi << 24) + 
                       (le.HighWord.Bits.BaseMid << 16) + le.BaseLow + addr->Offset);
        break;
    case AddrModeFlat:
        return (void*)addr->Offset;
    }
    return NULL;
}

static unsigned be_i386_build_addr(HANDLE hThread, const CONTEXT* ctx, ADDRESS* addr,
                                   unsigned seg, unsigned long offset)
{
    addr->Mode    = AddrModeFlat;
    addr->Segment = seg;
    addr->Offset  = offset;
    if (seg)
    {
        addr->Mode = get_selector_type(hThread, ctx, seg);
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

static unsigned be_i386_get_addr(HANDLE hThread, const CONTEXT* ctx, 
                                 enum be_cpu_addr bca, ADDRESS* addr)
{
    switch (bca)
    {
    case be_cpu_addr_pc:
        return be_i386_build_addr(hThread, ctx, addr, ctx->SegCs, ctx->Eip);
    case be_cpu_addr_stack:
        return be_i386_build_addr(hThread, ctx, addr, ctx->SegSs, ctx->Esp);
    case be_cpu_addr_frame:
        return be_i386_build_addr(hThread, ctx, addr, ctx->SegSs, ctx->Ebp);
    }
    return FALSE;
}

static void be_i386_single_step(CONTEXT* ctx, unsigned enable)
{
    if (enable) ctx->EFlags |= STEP_FLAG;
    else ctx->EFlags &= ~STEP_FLAG;
}

static void be_i386_print_context(HANDLE hThread, const CONTEXT* ctx)
{
    char        buf[33];
    char*       pt;

    dbg_printf("Register dump:\n");

    /* First get the segment registers out of the way */
    dbg_printf(" CS:%04x SS:%04x DS:%04x ES:%04x FS:%04x GS:%04x",
               (WORD)ctx->SegCs, (WORD)ctx->SegSs,
               (WORD)ctx->SegDs, (WORD)ctx->SegEs,
               (WORD)ctx->SegFs, (WORD)ctx->SegGs);

    strcpy(buf, "   - 00      - - - ");
    pt = buf + strlen(buf) - 1;
    if (ctx->EFlags & 0x00000001) *pt-- = 'C'; /* Carry Flag */
    if (ctx->EFlags & 0x00000002) *pt-- = '1';
    if (ctx->EFlags & 0x00000004) *pt-- = 'P'; /* Parity Flag */
    if (ctx->EFlags & 0x00000008) *pt-- = '-';
    if (ctx->EFlags & 0x00000010) *pt-- = 'A'; /* Auxiliary Carry Flag */
    if (ctx->EFlags & 0x00000020) *pt-- = '-';
    if (ctx->EFlags & 0x00000040) *pt-- = 'Z'; /* Zero Flag */
    if (ctx->EFlags & 0x00000080) *pt-- = 'S'; /* Sign Flag */
    if (ctx->EFlags & 0x00000100) *pt-- = 'T'; /* Trap/Trace Flag */
    if (ctx->EFlags & 0x00000200) *pt-- = 'I'; /* Interupt Enable Flag */
    if (ctx->EFlags & 0x00000400) *pt-- = 'D'; /* Direction Indicator */
    if (ctx->EFlags & 0x00000800) *pt-- = 'O'; /* Overflow flags */
    if (ctx->EFlags & 0x00001000) *pt-- = '1'; /* I/O Privilege Level */
    if (ctx->EFlags & 0x00002000) *pt-- = '1'; /* I/O Privilege Level */
    if (ctx->EFlags & 0x00004000) *pt-- = 'N'; /* Nested Task Flag */
    if (ctx->EFlags & 0x00008000) *pt-- = '-';
    if (ctx->EFlags & 0x00010000) *pt-- = 'R'; /* Resume Flag */
    if (ctx->EFlags & 0x00020000) *pt-- = 'V'; /* Vritual Mode Flag */
    if (ctx->EFlags & 0x00040000) *pt-- = 'a'; /* Alignment Check Flag */
    
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
}

static void be_i386_print_segment_info(HANDLE hThread, const CONTEXT* ctx)
{
    if (get_selector_type(hThread, ctx, ctx->SegCs) == AddrMode1616)
    {
        info_win32_segments(ctx->SegDs >> 3, 1);
        if (ctx->SegEs != ctx->SegDs) info_win32_segments(ctx->SegEs >> 3, 1);
    }
    info_win32_segments(ctx->SegFs >> 3, 1);
}

static struct dbg_internal_var be_i386_ctx[] =
{
    {CV_REG_AL,         "AL",           (DWORD*)FIELD_OFFSET(CONTEXT, Eax),     dbg_itype_unsigned_char_int},
    {CV_REG_CL,         "CL",           (DWORD*)FIELD_OFFSET(CONTEXT, Ecx),     dbg_itype_unsigned_char_int},
    {CV_REG_DL,         "DL",           (DWORD*)FIELD_OFFSET(CONTEXT, Edx),     dbg_itype_unsigned_char_int},
    {CV_REG_BL,         "BL",           (DWORD*)FIELD_OFFSET(CONTEXT, Ebx),     dbg_itype_unsigned_char_int},
    {CV_REG_AH,         "AH",           (DWORD*)(FIELD_OFFSET(CONTEXT, Eax)+1), dbg_itype_unsigned_char_int},
    {CV_REG_CH,         "CH",           (DWORD*)(FIELD_OFFSET(CONTEXT, Ecx)+1), dbg_itype_unsigned_char_int},
    {CV_REG_DH,         "DH",           (DWORD*)(FIELD_OFFSET(CONTEXT, Edx)+1), dbg_itype_unsigned_char_int},
    {CV_REG_BH,         "BH",           (DWORD*)(FIELD_OFFSET(CONTEXT, Ebx)+1), dbg_itype_unsigned_char_int},
    {CV_REG_AX,         "AX",           (DWORD*)FIELD_OFFSET(CONTEXT, Eax),     dbg_itype_unsigned_short_int},
    {CV_REG_CX,         "CX",           (DWORD*)FIELD_OFFSET(CONTEXT, Ecx),     dbg_itype_unsigned_short_int},
    {CV_REG_DX,         "DX",           (DWORD*)FIELD_OFFSET(CONTEXT, Edx),     dbg_itype_unsigned_short_int},
    {CV_REG_BX,         "BX",           (DWORD*)FIELD_OFFSET(CONTEXT, Ebx),     dbg_itype_unsigned_short_int},
    {CV_REG_SP,         "SP",           (DWORD*)FIELD_OFFSET(CONTEXT, Esp),     dbg_itype_unsigned_short_int},
    {CV_REG_BP,         "BP",           (DWORD*)FIELD_OFFSET(CONTEXT, Ebp),     dbg_itype_unsigned_short_int},
    {CV_REG_SI,         "SI",           (DWORD*)FIELD_OFFSET(CONTEXT, Esi),     dbg_itype_unsigned_short_int},
    {CV_REG_DI,         "DI",           (DWORD*)FIELD_OFFSET(CONTEXT, Edi),     dbg_itype_unsigned_short_int},
    {CV_REG_EAX,        "EAX",          (DWORD*)FIELD_OFFSET(CONTEXT, Eax),     dbg_itype_unsigned_int},
    {CV_REG_ECX,        "ECX",          (DWORD*)FIELD_OFFSET(CONTEXT, Ecx),     dbg_itype_unsigned_int},
    {CV_REG_EDX,        "EDX",          (DWORD*)FIELD_OFFSET(CONTEXT, Edx),     dbg_itype_unsigned_int},
    {CV_REG_EBX,        "EBX",          (DWORD*)FIELD_OFFSET(CONTEXT, Ebx),     dbg_itype_unsigned_int},
    {CV_REG_ESP,        "ESP",          (DWORD*)FIELD_OFFSET(CONTEXT, Esp),     dbg_itype_unsigned_int},
    {CV_REG_EBP,        "EBP",          (DWORD*)FIELD_OFFSET(CONTEXT, Ebp),     dbg_itype_unsigned_int},
    {CV_REG_ESI,        "ESI",          (DWORD*)FIELD_OFFSET(CONTEXT, Esi),     dbg_itype_unsigned_int},
    {CV_REG_EDI,        "EDI",          (DWORD*)FIELD_OFFSET(CONTEXT, Edi),     dbg_itype_unsigned_int},
    {CV_REG_ES,         "ES",           (DWORD*)FIELD_OFFSET(CONTEXT, SegEs),   dbg_itype_unsigned_short_int},
    {CV_REG_CS,         "CS",           (DWORD*)FIELD_OFFSET(CONTEXT, SegCs),   dbg_itype_unsigned_short_int},
    {CV_REG_SS,         "SS",           (DWORD*)FIELD_OFFSET(CONTEXT, SegSs),   dbg_itype_unsigned_short_int},
    {CV_REG_DS,         "DS",           (DWORD*)FIELD_OFFSET(CONTEXT, SegDs),   dbg_itype_unsigned_short_int},
    {CV_REG_FS,         "FS",           (DWORD*)FIELD_OFFSET(CONTEXT, SegFs),   dbg_itype_unsigned_short_int},
    {CV_REG_GS,         "GS",           (DWORD*)FIELD_OFFSET(CONTEXT, SegGs),   dbg_itype_unsigned_short_int},
    {CV_REG_IP,         "IP",           (DWORD*)FIELD_OFFSET(CONTEXT, Eip),     dbg_itype_unsigned_short_int},
    {CV_REG_FLAGS,      "FLAGS",        (DWORD*)FIELD_OFFSET(CONTEXT, EFlags),  dbg_itype_unsigned_short_int},
    {CV_REG_EIP,        "EIP",          (DWORD*)FIELD_OFFSET(CONTEXT, Eip),     dbg_itype_unsigned_int},
    {CV_REG_EFLAGS,     "EFLAGS",       (DWORD*)FIELD_OFFSET(CONTEXT, EFlags),  dbg_itype_unsigned_int},
    {0,                 NULL,           0,                                      dbg_itype_none}
};

static const struct dbg_internal_var* be_i386_init_registers(CONTEXT* ctx)
{
    struct dbg_internal_var*    div;

    for (div = be_i386_ctx; div->name; div++) 
        div->pval = (DWORD*)((char*)ctx + (DWORD)div->pval);
    return be_i386_ctx;
}

static unsigned be_i386_is_step_over_insn(const void* insn)
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

static unsigned be_i386_is_function_return(const void* insn)
{
    BYTE ch;

    if (!dbg_read_memory(insn, &ch, sizeof(ch))) return FALSE;
    return (ch == 0xC2) || (ch == 0xC3);
}

static unsigned be_i386_is_break_insn(const void* insn)
{
    BYTE        c;

    if (!dbg_read_memory(insn, &c, sizeof(c))) return FALSE;
    return c == 0xCC;
}

static unsigned be_i386_is_func_call(const void* insn, ADDRESS* callee)
{
    BYTE        ch;
    int         delta;

    if (!dbg_read_memory(insn, &ch, sizeof(ch))) return FALSE;
    switch (ch)
    {
    case 0xe8:
	dbg_read_memory((const char*)insn + 1, &delta, sizeof(delta));
	
        callee->Mode = AddrModeFlat;
        callee->Offset = (DWORD)insn;
	be_i386_disasm_one_insn(callee, FALSE);
        callee->Offset += delta;

        return TRUE;
    case 0xff:
        if (!dbg_read_memory((const char*)insn + 1, &ch, sizeof(ch)))
            return FALSE;
        ch &= 0x38;
        if (ch != 0x10 && ch != 0x18) return FALSE;
        /* fall through */
    case 0x9c:
    case 0xCD:
        WINE_FIXME("Unsupported yet call insn (0x%02x) at %p\n", ch, insn);
        /* fall through */
    default:
        return FALSE;
    }
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

static inline int be_i386_get_unused_DR(CONTEXT* ctx, unsigned long** r)
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

static unsigned be_i386_insert_Xpoint(HANDLE hProcess, const struct be_process_io* pio, 
                                      CONTEXT* ctx, enum be_xpoint_type type, 
                                      void* addr, unsigned long* val, unsigned size)
{
    unsigned char       ch;
    unsigned long       sz;
    unsigned long*      pr;
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
        if ((reg = be_i386_get_unused_DR(ctx, &pr)) == -1) return 0;
        *pr = (unsigned long)addr;
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

static unsigned be_i386_remove_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                      CONTEXT* ctx, enum be_xpoint_type type, 
                                      void* addr, unsigned long val, unsigned size)
{
    unsigned long       sz;
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

static unsigned be_i386_is_watchpoint_set(const CONTEXT* ctx, unsigned idx)
{
    return ctx->Dr6 & (1 << idx);
}

static void be_i386_clear_watchpoint(CONTEXT* ctx, unsigned idx)
{
    ctx->Dr6 &= ~(1 << idx);
}

static int be_i386_adjust_pc_for_break(CONTEXT* ctx, BOOL way)
{
    if (way)
    {
        ctx->Eip--;
        return -1;
    }
    ctx->Eip++;
    return 1;
}

static int be_i386_fetch_integer(const struct dbg_lvalue* lvalue, unsigned size,
                                 unsigned ext_sign, LONGLONG* ret)
{
    if (size != 1 && size != 2 && size != 4 && size != 8) return FALSE;

    memset(ret, 0, sizeof(*ret)); /* clear unread bytes */
    /* FIXME: this assumes that debuggee and debugger use the same
     * integral representation
     */
    if (!memory_read_value(lvalue, size, ret)) return FALSE;

    /* propagate sign information */
    if (ext_sign && size < 8 && (*ret >> (size * 8 - 1)) != 0)
    {
        ULONGLONG neg = -1;
        *ret |= neg << (size * 8);
    }
    return TRUE;
}

static int be_i386_fetch_float(const struct dbg_lvalue* lvalue, unsigned size, 
                               long double* ret)
{
    char        tmp[12];

    /* FIXME: this assumes that debuggee and debugger use the same 
     * representation for reals
     */
    if (!memory_read_value(lvalue, size, tmp)) return FALSE;

    /* float & double types have to be promoted to a long double */
    switch (size)
    {
    case sizeof(float):         *ret = *(float*)tmp;            break;
    case sizeof(double):        *ret = *(double*)tmp;           break;
    case sizeof(long double):   *ret = *(long double*)tmp;      break;
    default:                    return FALSE;
    }
    return TRUE;
}

struct backend_cpu be_i386 =
{
    be_i386_linearize,
    be_i386_build_addr,
    be_i386_get_addr,
    be_i386_single_step,
    be_i386_print_context,
    be_i386_print_segment_info,
    be_i386_init_registers,
    be_i386_is_step_over_insn,
    be_i386_is_function_return,
    be_i386_is_break_insn,
    be_i386_is_func_call,
    be_i386_disasm_one_insn,
    be_i386_insert_Xpoint,
    be_i386_remove_Xpoint,
    be_i386_is_watchpoint_set,
    be_i386_clear_watchpoint,
    be_i386_adjust_pc_for_break,
    be_i386_fetch_integer,
    be_i386_fetch_float,
};
#endif
