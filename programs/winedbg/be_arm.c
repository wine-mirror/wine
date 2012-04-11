/*
 * Debugger ARM specific functions
 *
 * Copyright 2000-2003 Marcus Meissner
 *                2004 Eric Pouech
 *                2010, 2011 André Hentschel
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

#if defined(__arm__) && !defined(__ARMEB__)

/*
 * Switch to disassemble Thumb code.
 */
static BOOL db_disasm_thumb = FALSE;

/*
 * Flag to indicate whether we need to display instruction,
 * or whether we just need to know the address of the next
 * instruction.
 */
static BOOL db_display = FALSE;

#define ARM_INSN_SIZE    4
#define THUMB_INSN_SIZE  2

#define ROR32(n, r) (((n) >> (r)) | ((n) << (32 - (r))))

#define get_cond(ins)           tbl_cond[(ins >> 28) & 0x0f]
#define get_nibble(ins, num)    ((ins >> (num * 4)) & 0x0f)

static char const tbl_addrmode[][3] = {
    "da", "ia", "db", "ib"
};

static char const tbl_cond[][3] = {
    "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc", "hi", "ls", "ge", "lt", "gt", "le", "", ""
};

static char const tbl_dataops[][4] = {
    "and", "eor", "sub", "rsb", "add", "adc", "sbc", "rsc", "tst", "teq", "cmp", "cmn", "orr",
    "mov", "bic", "mvn"
};

static UINT db_get_inst(void* addr, int size)
{
    UINT result = 0;
    char buffer[4];

    if (dbg_read_memory(addr, buffer, size))
    {
        switch (size)
        {
        case 4:
            result = *(UINT*)buffer;
            break;
        case 2:
            result = *(WORD*)buffer;
            break;
        }
    }
    return result;
}

static UINT arm_disasm_branch(UINT inst)
{
    short link = (inst >> 24) & 0x01;
    int offset = (inst << 2) & 0x03ffffff;

    if (offset & 0x02000000) offset |= 0xfc000000;
    offset += 8;

    dbg_printf("\n\tb%s%s\t#%d/0x%08x", link ? "l" : "", get_cond(inst), offset, offset);
    return 0;
}

static UINT arm_disasm_dataprocessing(UINT inst)
{
    short condcodes = (inst >> 20) & 0x01;
    short opcode    = (inst >> 21) & 0x0f;
    short immediate = (inst >> 25) & 0x01;
    short no_op1    = (opcode & 0x0d) == 0x0d;

    /* check for nop */
    if (get_nibble(inst, 3) == 15 /* r15 */ && condcodes == 0 &&
        opcode >= 8 /* tst */ && opcode <= 11 /* cmn */)
    {
        dbg_printf("\n\tnop");
        return 0;
    }

    dbg_printf("\n\t%s%s%s", tbl_dataops[opcode], condcodes ? "s" : "", get_cond(inst));
    if (no_op1)
    {
        if (immediate)
            dbg_printf("\tr%u, #%u", get_nibble(inst, 3),
                       ROR32(inst & 0xff, 2 * get_nibble(inst, 2)));
        else
            dbg_printf("\tr%u, r%u", get_nibble(inst, 3), get_nibble(inst, 0));
    }
    else
    {
        if (immediate)
            dbg_printf("\tr%u, r%u, #%u", get_nibble(inst, 3), get_nibble(inst, 4),
                       ROR32(inst & 0xff, 2 * get_nibble(inst, 2)));
        else
            dbg_printf("\tr%u, r%u, r%u", get_nibble(inst, 3), get_nibble(inst, 4),
                       get_nibble(inst, 0));
    }
    return 0;
}

static UINT arm_disasm_singletrans(UINT inst)
{
    short load      = (inst >> 20) & 0x01;
    short writeback = (inst >> 21) & 0x01;
    short byte      = (inst >> 22) & 0x01;
    short direction = (inst >> 23) & 0x01;
    /* FIXME: what to do with bit 24 (indexing) */
    short immediate = !((inst >> 25) & 0x01);
    short offset    = inst & 0x0fff;

    if (!direction) offset *= -1;

    dbg_printf("\n\t%s%s%s%s", load ? "ldr" : "str", byte ? "b" : "", writeback ? "t" : "",
               get_cond(inst));
    if (immediate)
        dbg_printf("\tr%u, [r%u, #%d]", get_nibble(inst, 3), get_nibble(inst, 4), offset);
    else
        dbg_printf("\tr%u, r%u, r%u", get_nibble(inst, 3), get_nibble(inst, 4),
                   get_nibble(inst, 0));
    return 0;
}

static UINT arm_disasm_halfwordtrans(UINT inst)
{
    short halfword  = (inst >> 5)  & 0x01;
    short sign      = (inst >> 6)  & 0x01;
    short load      = (inst >> 20) & 0x01;
    short writeback = (inst >> 21) & 0x01;
    short immediate = (inst >> 22) & 0x01;
    short direction = (inst >> 23) & 0x01;
    /* FIXME: what to do with bit 24 (indexing) */
    short offset    = ((inst >> 4) & 0xf0) + (inst & 0x0f);

    if (!direction) offset *= -1;

    dbg_printf("\n\t%s%s%s%s%s", load ? "ldr" : "str", sign ? "s" : "",
               halfword ? "h" : (sign ? "b" : ""), writeback ? "t" : "", get_cond(inst));
    if (immediate)
        dbg_printf("\tr%u, r%u, #%d", get_nibble(inst, 3), get_nibble(inst, 4), offset);
    else
        dbg_printf("\tr%u, r%u, r%u", get_nibble(inst, 3), get_nibble(inst, 4), get_nibble(inst, 0));
    return 0;
}

static UINT arm_disasm_blocktrans(UINT inst)
{
    short load      = (inst >> 20) & 0x01;
    short writeback = (inst >> 21) & 0x01;
    short psr       = (inst >> 22) & 0x01;
    short addrmode  = (inst >> 23) & 0x03;
    short i;
    short last=15;
    for (i=15;i>=0;i--)
        if ((inst>>i) & 1)
        {
            last = i;
            break;
        }

    dbg_printf("\n\t%s%s%s\tr%u%s, {", load ? "ldm" : "stm", tbl_addrmode[addrmode], get_cond(inst),
               get_nibble(inst, 4), writeback ? "!" : "");
    for (i=0;i<=15;i++)
        if ((inst>>i) & 1)
        {
            if (i == last) dbg_printf("r%u", i);
            else dbg_printf("r%u, ", i);
        }
    dbg_printf("}%s", psr ? "^" : "");
    return 0;
}

static UINT arm_disasm_swi(UINT inst)
{
    UINT comment = inst & 0x00ffffff;
    dbg_printf("\n\tswi%s\t#%d/0x%08x", get_cond(inst), comment, comment);
    return 0;
}

static UINT arm_disasm_coproctrans(UINT inst)
{
    WORD CRm    = inst & 0x0f;
    WORD CP     = (inst >> 5)  & 0x07;
    WORD CPnum  = (inst >> 8)  & 0x0f;
    WORD CRn    = (inst >> 16) & 0x0f;
    WORD load   = (inst >> 20) & 0x01;
    WORD CP_Opc = (inst >> 21) & 0x07;

    dbg_printf("\n\t%s%s\t%u, %u, r%u, cr%u, cr%u, {%u}", load ? "mrc" : "mcr", get_cond(inst), CPnum,
               CP, get_nibble(inst, 3), CRn, CRm, CP_Opc);
    return 0;
}

static UINT arm_disasm_coprocdataop(UINT inst)
{
    WORD CRm    = inst & 0x0f;
    WORD CP     = (inst >> 5)  & 0x07;
    WORD CPnum  = (inst >> 8)  & 0x0f;
    WORD CRd    = (inst >> 12) & 0x0f;
    WORD CRn    = (inst >> 16) & 0x0f;
    WORD CP_Opc = (inst >> 20) & 0x0f;

    dbg_printf("\n\tcdp%s\t%u, %u, cr%u, cr%u, cr%u, {%u}", get_cond(inst),
               CPnum, CP, CRd, CRn, CRm, CP_Opc);
    return 0;
}

static UINT arm_disasm_coprocdatatrans(UINT inst)
{
    WORD CPnum  = (inst >> 8)  & 0x0f;
    WORD CRd    = (inst >> 12) & 0x0f;
    WORD load      = (inst >> 20) & 0x01;
    /* FIXME: what to do with bit 21 (writeback) */
    WORD translen  = (inst >> 22) & 0x01;
    WORD direction = (inst >> 23) & 0x01;
    /* FIXME: what to do with bit 24 (indexing) */
    short offset    = (inst & 0xff) << 2;

    if (!direction) offset *= -1;

    dbg_printf("\n\t%s%s%s", load ? "ldc" : "stc", translen ? "l" : "", get_cond(inst));
    dbg_printf("\t%u, cr%u, [r%u, #%d]", CPnum, CRd, get_nibble(inst, 4), offset);
    return 0;
}

struct inst_arm
{
        UINT mask;
        UINT pattern;
        UINT (*func)(UINT);
};

static const struct inst_arm tbl_arm[] = {
    { 0x0e000000, 0x0a000000, arm_disasm_branch },
    { 0x0c000000, 0x00000000, arm_disasm_dataprocessing },
    { 0x0c000000, 0x04000000, arm_disasm_singletrans },
    { 0x0e000090, 0x00000090, arm_disasm_halfwordtrans },
    { 0x0e000000, 0x08000000, arm_disasm_blocktrans },
    { 0x0f000000, 0x0f000000, arm_disasm_swi },
    { 0x0f000010, 0x0e000010, arm_disasm_coproctrans },
    { 0x0f000010, 0x0e000000, arm_disasm_coprocdataop },
    { 0x0e000000, 0x0c000000, arm_disasm_coprocdatatrans },
    { 0x00000000, 0x00000000, NULL }
};

/***********************************************************************
 *              disasm_one_insn
 *
 * Disassemble instruction at 'addr'. addr is changed to point to the
 * start of the next instruction.
 */
void be_arm_disasm_one_insn(ADDRESS64 *addr, int display)
{
    struct inst_arm *a_ptr = (struct inst_arm *)&tbl_arm;
    UINT inst;
    int size;
    int matched = 0;

    char tmp[64];
    DWORD_PTR* pval;

    if (!memory_get_register(CV_ARM_CPSR, &pval, tmp, sizeof(tmp)))
        dbg_printf("\n\tmemory_get_register failed: %s\n",tmp);
    else
        db_disasm_thumb=(*pval & 0x20)?TRUE:FALSE;

    if (db_disasm_thumb) size = THUMB_INSN_SIZE;
    else size = ARM_INSN_SIZE;

    db_display = display;
    inst = db_get_inst( memory_to_linear_addr(addr), size );

    if (!db_disasm_thumb)
    {
        while (a_ptr->func) {
                if ((inst & a_ptr->mask) ==  a_ptr->pattern) {
                        matched = 1;
                        break;
                }
                a_ptr++;
        }

        if (!matched) {
                dbg_printf("\n\tUnknown Instruction: %08x\n", inst);
                addr->Offset += size;
                return;
        }
        else
        {
            if (!a_ptr->func(inst))
            {
                dbg_printf("\n");
                addr->Offset += size;
            }
            return;
        }
    }
    else
    {
        dbg_printf("\n\tThumb disassembling not yet implemented\n");
        addr->Offset += size;
    }
}

static unsigned be_arm_get_addr(HANDLE hThread, const CONTEXT* ctx,
                                enum be_cpu_addr bca, ADDRESS64* addr)
{
    switch (bca)
    {
    case be_cpu_addr_pc:
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->Pc);
    case be_cpu_addr_stack:
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->Sp);
    case be_cpu_addr_frame:
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->Fp);
    }
    return FALSE;
}

static unsigned be_arm_get_register_info(int regno, enum be_cpu_addr* kind)
{
    switch (regno)
    {
    case CV_ARM_PC:  *kind = be_cpu_addr_pc; return TRUE;
    case CV_ARM_R0 + 11: *kind = be_cpu_addr_frame; return TRUE;
    case CV_ARM_SP:  *kind = be_cpu_addr_stack; return TRUE;
    }
    return FALSE;
}

static void be_arm_single_step(CONTEXT* ctx, unsigned enable)
{
    dbg_printf("be_arm_single_step: not done\n");
}

static void be_arm_print_context(HANDLE hThread, const CONTEXT* ctx, int all_regs)
{
    static const char condflags[] = "NZCV";
    int i;
    char        buf[8];

    switch (ctx->Cpsr & 0x1F)
    {
    case 0:  strcpy(buf, "User26"); break;
    case 1:  strcpy(buf, "FIQ26"); break;
    case 2:  strcpy(buf, "IRQ26"); break;
    case 3:  strcpy(buf, "SVC26"); break;
    case 16: strcpy(buf, "User"); break;
    case 17: strcpy(buf, "FIQ"); break;
    case 18: strcpy(buf, "IRQ"); break;
    case 19: strcpy(buf, "SVC"); break;
    case 23: strcpy(buf, "ABT"); break;
    case 27: strcpy(buf, "UND"); break;
    default: strcpy(buf, "UNKNWN"); break;
    }

    dbg_printf("Register dump:\n");
    dbg_printf("%s %s Mode\n", (ctx->Cpsr & 0x20) ? "Thumb" : "ARM", buf);

    strcpy(buf, condflags);
    for (i = 0; buf[i]; i++)
        if (!((ctx->Cpsr >> 26) & (1 << (sizeof(condflags) - i))))
            buf[i] = '-';

    dbg_printf(" Pc:%04x Sp:%04x Lr:%04x Cpsr:%04x(%s)\n",
               ctx->Pc, ctx->Sp, ctx->Lr, ctx->Cpsr, buf);
    dbg_printf(" r0:%04x r1:%04x r2:%04x r3:%04x\n",
               ctx->R0, ctx->R1, ctx->R2, ctx->R3);
    dbg_printf(" r4:%04x r5:%04x  r6:%04x  r7:%04x r8:%04x\n",
               ctx->R4, ctx->R5, ctx->R6, ctx->R7, ctx->R8 );
    dbg_printf(" r9:%04x r10:%04x Fp:%04x Ip:%04x\n",
               ctx->R9, ctx->R10, ctx->Fp, ctx->Ip );

    if (all_regs) dbg_printf( "Floating point ARM dump not implemented\n" );
}

static void be_arm_print_segment_info(HANDLE hThread, const CONTEXT* ctx)
{
}

static struct dbg_internal_var be_arm_ctx[] =
{
    {CV_ARM_R0 +  0,    "r0",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R0),     dbg_itype_unsigned_int},
    {CV_ARM_R0 +  1,    "r1",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R1),     dbg_itype_unsigned_int},
    {CV_ARM_R0 +  2,    "r2",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R2),     dbg_itype_unsigned_int},
    {CV_ARM_R0 +  3,    "r3",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R3),     dbg_itype_unsigned_int},
    {CV_ARM_R0 +  4,    "r4",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R4),     dbg_itype_unsigned_int},
    {CV_ARM_R0 +  5,    "r5",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R5),     dbg_itype_unsigned_int},
    {CV_ARM_R0 +  6,    "r6",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R6),     dbg_itype_unsigned_int},
    {CV_ARM_R0 +  7,    "r7",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R7),     dbg_itype_unsigned_int},
    {CV_ARM_R0 +  8,    "r8",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R8),     dbg_itype_unsigned_int},
    {CV_ARM_R0 +  9,    "r9",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R9),     dbg_itype_unsigned_int},
    {CV_ARM_R0 +  10,   "r10",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, R10),    dbg_itype_unsigned_int},
    {CV_ARM_R0 +  11,   "r11",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Fp),     dbg_itype_unsigned_int},
    {CV_ARM_R0 +  12,   "r12",          (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Ip),     dbg_itype_unsigned_int},
    {CV_ARM_SP,         "sp",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Sp),     dbg_itype_unsigned_int},
    {CV_ARM_LR,         "lr",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Lr),     dbg_itype_unsigned_int},
    {CV_ARM_PC,         "pc",           (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Pc),     dbg_itype_unsigned_int},
    {CV_ARM_CPSR,       "cpsr",         (DWORD_PTR*)FIELD_OFFSET(CONTEXT, Cpsr),   dbg_itype_unsigned_int},
    {0,                 NULL,           0,                                         dbg_itype_none}
};

static unsigned be_arm_is_step_over_insn(const void* insn)
{
    dbg_printf("be_arm_is_step_over_insn: not done\n");
    return FALSE;
}

static unsigned be_arm_is_function_return(const void* insn)
{
    dbg_printf("be_arm_is_function_return: not done\n");
    return FALSE;
}

static unsigned be_arm_is_break_insn(const void* insn)
{
    dbg_printf("be_arm_is_break_insn: not done\n");
    return FALSE;
}

static unsigned be_arm_is_func_call(const void* insn, ADDRESS64* callee)
{
    return FALSE;
}

static unsigned be_arm_is_jump(const void* insn, ADDRESS64* jumpee)
{
    return FALSE;
}

static unsigned be_arm_insert_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                     CONTEXT* ctx, enum be_xpoint_type type,
                                     void* addr, unsigned long* val, unsigned size)
{
    SIZE_T              sz;

    switch (type)
    {
    case be_xpoint_break:
        if (!size) return 0;
        if (!pio->read(hProcess, addr, val, 4, &sz) || sz != 4) return 0;
    default:
        dbg_printf("Unknown/unsupported bp type %c\n", type);
        return 0;
    }
    return 1;
}

static unsigned be_arm_remove_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                     CONTEXT* ctx, enum be_xpoint_type type,
                                     void* addr, unsigned long val, unsigned size)
{
    SIZE_T              sz;

    switch (type)
    {
    case be_xpoint_break:
        if (!size) return 0;
        if (!pio->write(hProcess, addr, &val, 4, &sz) || sz == 4) return 0;
        break;
    default:
        dbg_printf("Unknown/unsupported bp type %c\n", type);
        return 0;
    }
    return 1;
}

static unsigned be_arm_is_watchpoint_set(const CONTEXT* ctx, unsigned idx)
{
    dbg_printf("be_arm_is_watchpoint_set: not done\n");
    return FALSE;
}

static void be_arm_clear_watchpoint(CONTEXT* ctx, unsigned idx)
{
    dbg_printf("be_arm_clear_watchpoint: not done\n");
}

static int be_arm_adjust_pc_for_break(CONTEXT* ctx, BOOL way)
{
    INT step = (ctx->Cpsr & 0x20) ? 2 : 4;

    if (way)
    {
        ctx->Pc -= step;
        return -step;
    }
    ctx->Pc += step;
    return step;
}

static int be_arm_fetch_integer(const struct dbg_lvalue* lvalue, unsigned size,
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

static int be_arm_fetch_float(const struct dbg_lvalue* lvalue, unsigned size,
                              long double* ret)
{
    char        tmp[sizeof(long double)];

    /* FIXME: this assumes that debuggee and debugger use the same
     * representation for reals
     */
    if (!memory_read_value(lvalue, size, tmp)) return FALSE;

    switch (size)
    {
    case sizeof(float):         *ret = *(float*)tmp;            break;
    case sizeof(double):        *ret = *(double*)tmp;           break;
    default:                    return FALSE;
    }
    return TRUE;
}

static int be_arm_store_integer(const struct dbg_lvalue* lvalue, unsigned size,
                                unsigned is_signed, LONGLONG val)
{
    /* this is simple if we're on a little endian CPU */
    return memory_write_value(lvalue, size, &val);
}

struct backend_cpu be_arm =
{
    IMAGE_FILE_MACHINE_ARMV7,
    4,
    be_cpu_linearize,
    be_cpu_build_addr,
    be_arm_get_addr,
    be_arm_get_register_info,
    be_arm_single_step,
    be_arm_print_context,
    be_arm_print_segment_info,
    be_arm_ctx,
    be_arm_is_step_over_insn,
    be_arm_is_function_return,
    be_arm_is_break_insn,
    be_arm_is_func_call,
    be_arm_is_jump,
    be_arm_disasm_one_insn,
    be_arm_insert_Xpoint,
    be_arm_remove_Xpoint,
    be_arm_is_watchpoint_set,
    be_arm_clear_watchpoint,
    be_arm_adjust_pc_for_break,
    be_arm_fetch_integer,
    be_arm_fetch_float,
    be_arm_store_integer,
};
#endif
