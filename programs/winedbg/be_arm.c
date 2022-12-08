/*
 * Debugger ARM specific functions
 *
 * Copyright 2010-2013 André Hentschel
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
#define THUMB2_INSN_SIZE 4

#define ROR32(n, r) (((n) >> (r)) | ((n) << (32 - (r))))

#define get_cond(ins)           tbl_cond[(ins >> 28) & 0x0f]
#define get_nibble(ins, num)    ((ins >> (num * 4)) & 0x0f)

static char const tbl_regs[][4] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10",
    "fp", "ip", "sp", "lr", "pc", "cpsr"
};

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

static char const tbl_shifts[][4] = {
    "lsl", "lsr", "asr", "ror"
};

static char const tbl_hiops_t[][4] = {
    "add", "cmp", "mov", "bx"
};

static char const tbl_aluops_t[][4] = {
    "and", "eor", "lsl", "lsr", "asr", "adc", "sbc", "ror", "tst", "neg", "cmp", "cmn", "orr",
    "mul", "bic", "mvn"
};

static char const tbl_immops_t[][4] = {
    "mov", "cmp", "add", "sub"
};

static char const tbl_sregops_t[][5] = {
    "strh", "ldsb", "ldrh", "ldsh"
};

static char const tbl_miscops_t2[][6] = {
    "rev", "rev16", "rbit", "revsh"
};

static char const tbl_width_t2[][2] = {
    "b", "h", "", "?"
};

static char const tbl_special_regs_t2[][12] = {
    "apsr", "iapsr", "eapsr", "xpsr", "rsvd", "ipsr", "epsr", "iepsr", "msp", "psp", "rsvd", "rsvd",
    "rsvd", "rsvd", "rsvd", "rsvd", "primask", "basepri", "basepri_max", "faultmask", "control"
};

static char const tbl_hints_t2[][6] = {
    "nop", "yield", "wfe", "wfi", "sev"
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

static void db_printsym(unsigned int addr)
{
    ADDRESS64   a;

    a.Mode   = AddrModeFlat;
    a.Offset = addr;

    print_address(&a, TRUE);
}

static UINT arm_disasm_branch(UINT inst, ADDRESS64 *addr)
{
    short link = (inst >> 24) & 0x01;
    int offset = (inst << 2) & 0x03ffffff;

    if (offset & 0x02000000) offset |= 0xfc000000;
    offset += 8;

    dbg_printf("\n\tb%s%s\t", link ? "l" : "", get_cond(inst));
    db_printsym(addr->Offset + offset);
    return 0;
}

static UINT arm_disasm_mul(UINT inst, ADDRESS64 *addr)
{
    short accu = (inst >> 21) & 0x01;
    short condcodes = (inst >> 20) & 0x01;

    if (accu)
        dbg_printf("\n\tmla%s%s\t%s, %s, %s, %s", get_cond(inst), condcodes ? "s" : "",
                   tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)],
                   tbl_regs[get_nibble(inst, 2)], tbl_regs[get_nibble(inst, 3)]);
    else
        dbg_printf("\n\tmul%s%s\t%s, %s, %s", get_cond(inst), condcodes ? "s" : "",
                   tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)],
                   tbl_regs[get_nibble(inst, 2)]);
    return 0;
}

static UINT arm_disasm_longmul(UINT inst, ADDRESS64 *addr)
{
    short sign = (inst >> 22) & 0x01;
    short accu = (inst >> 21) & 0x01;
    short condcodes = (inst >> 20) & 0x01;

    dbg_printf("\n\t%s%s%s%s\t%s, %s, %s, %s", sign ? "s" : "u", accu ? "mlal" : "mull",
               get_cond(inst), condcodes ? "s" : "",
               tbl_regs[get_nibble(inst, 3)], tbl_regs[get_nibble(inst, 4)],
               tbl_regs[get_nibble(inst, 0)], tbl_regs[get_nibble(inst, 2)]);
    return 0;
}

static UINT arm_disasm_swp(UINT inst, ADDRESS64 *addr)
{
    short byte = (inst >> 22) & 0x01;

    dbg_printf("\n\tswp%s%s\t%s, %s, [%s]", get_cond(inst), byte ? "b" : "",
               tbl_regs[get_nibble(inst, 3)], tbl_regs[get_nibble(inst, 0)],
               tbl_regs[get_nibble(inst, 4)]);
    return 0;
}

static UINT arm_disasm_halfwordtrans(UINT inst, ADDRESS64 *addr)
{
    short halfword  = (inst >> 5)  & 0x01;
    short sign      = (inst >> 6)  & 0x01;
    short load      = (inst >> 20) & 0x01;
    short writeback = (inst >> 21) & 0x01;
    short immediate = (inst >> 22) & 0x01;
    short direction = (inst >> 23) & 0x01;
    short indexing  = (inst >> 24) & 0x01;
    short offset    = ((inst >> 4) & 0xf0) + (inst & 0x0f);

    dbg_printf("\n\t%s%s%s%s%s", load ? "ldr" : "str", sign ? "s" : "",
               halfword ? "h" : (sign ? "b" : ""), writeback ? "t" : "", get_cond(inst));
    dbg_printf("\t%s, ", tbl_regs[get_nibble(inst, 3)]);
    if (indexing)
    {
        if (immediate)
            dbg_printf("[%s, #%s%d]", tbl_regs[get_nibble(inst, 4)], direction ? "" : "-", offset);
        else
            dbg_printf("[%s, %s]", tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)]);
    }
    else
    {
        if (immediate)
            dbg_printf("[%s], #%s%d", tbl_regs[get_nibble(inst, 4)], direction ? "" : "-", offset);
        else
            dbg_printf("[%s], %s", tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)]);
    }
    return 0;
}

static UINT arm_disasm_branchxchg(UINT inst, ADDRESS64 *addr)
{
    dbg_printf("\n\tbx%s\t%s", get_cond(inst), tbl_regs[get_nibble(inst, 0)]);
    return 0;
}

static UINT arm_disasm_mrstrans(UINT inst, ADDRESS64 *addr)
{
    short src = (inst >> 22) & 0x01;

    dbg_printf("\n\tmrs%s\t%s, %s", get_cond(inst), tbl_regs[get_nibble(inst, 3)],
               src ? "spsr" : "cpsr");
    return 0;
}

static UINT arm_disasm_msrtrans(UINT inst, ADDRESS64 *addr)
{
    short immediate = (inst >> 25) & 0x01;
    short dst = (inst >> 22) & 0x01;
    short simple = (inst >> 16) & 0x01;

    if (simple || !immediate)
    {
        dbg_printf("\n\tmsr%s\t%s, %s", get_cond(inst), dst ? "spsr" : "cpsr",
                   tbl_regs[get_nibble(inst, 0)]);
        return 0;
    }

    dbg_printf("\n\tmsr%s\t%s, #%u", get_cond(inst), dst ? "spsr" : "cpsr",
               ROR32(inst & 0xff, 2 * get_nibble(inst, 2)));
    return 0;
}

static UINT arm_disasm_wordmov(UINT inst, ADDRESS64 *addr)
{
    short top = (inst >> 22) & 0x01;

    dbg_printf("\n\tmov%s%s\t%s, #%u", top ? "t" : "w", get_cond(inst),
               tbl_regs[get_nibble(inst, 3)], (get_nibble(inst, 4) << 12) | (inst & 0x0fff));
    return 0;
}

static UINT arm_disasm_nop(UINT inst, ADDRESS64 *addr)
{
    dbg_printf("\n\tnop%s", get_cond(inst));
    return 0;
}

static UINT arm_disasm_dataprocessing(UINT inst, ADDRESS64 *addr)
{
    short condcodes = (inst >> 20) & 0x01;
    short opcode    = (inst >> 21) & 0x0f;
    short immediate = (inst >> 25) & 0x01;
    short no_op1    = (opcode & 0x0d) == 0x0d;
    short no_dst    = (opcode & 0x0c) == 0x08;

    dbg_printf("\n\t%s%s%s", tbl_dataops[opcode], condcodes ? "s" : "", get_cond(inst));
    if (!no_dst) dbg_printf("\t%s, ", tbl_regs[get_nibble(inst, 3)]);
    else dbg_printf("\t");

    if (no_op1)
    {
        if (immediate)
            dbg_printf("#%u", ROR32(inst & 0xff, 2 * get_nibble(inst, 2)));
        else
            dbg_printf("%s", tbl_regs[get_nibble(inst, 0)]);
    }
    else
    {
        if (immediate)
            dbg_printf("%s, #%u", tbl_regs[get_nibble(inst, 4)],
                       ROR32(inst & 0xff, 2 * get_nibble(inst, 2)));
        else if (((inst >> 4) & 0xff) == 0x00) /* no shift */
            dbg_printf("%s, %s", tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)]);
        else if (((inst >> 4) & 0x09) == 0x01) /* register shift */
            dbg_printf("%s, %s, %s %s", tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)],
                       tbl_shifts[(inst >> 5) & 0x03], tbl_regs[(inst >> 8) & 0x0f]);
        else if (((inst >> 4) & 0x01) == 0x00) /* immediate shift */
            dbg_printf("%s, %s, %s #%d", tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)],
                       tbl_shifts[(inst >> 5) & 0x03], (inst >> 7) & 0x1f);
        else
            return inst;
    }
    return 0;
}

static UINT arm_disasm_singletrans(UINT inst, ADDRESS64 *addr)
{
    short load      = (inst >> 20) & 0x01;
    short writeback = (inst >> 21) & 0x01;
    short byte      = (inst >> 22) & 0x01;
    short direction = (inst >> 23) & 0x01;
    short indexing  = (inst >> 24) & 0x01;
    short immediate = !((inst >> 25) & 0x01);
    short offset    = inst & 0x0fff;

    dbg_printf("\n\t%s%s%s%s", load ? "ldr" : "str", byte ? "b" : "", writeback ? "t" : "",
               get_cond(inst));
    dbg_printf("\t%s, ", tbl_regs[get_nibble(inst, 3)]);
    if (indexing)
    {
        if (immediate)
            dbg_printf("[%s, #%s%d]", tbl_regs[get_nibble(inst, 4)], direction ? "" : "-", offset);
        else if (((inst >> 4) & 0xff) == 0x00) /* no shift */
            dbg_printf("[%s, %s]", tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)]);
        else if (((inst >> 4) & 0x01) == 0x00) /* immediate shift (there's no register shift) */
            dbg_printf("[%s, %s, %s #%d]", tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)],
                       tbl_shifts[(inst >> 5) & 0x03], (inst >> 7) & 0x1f);
        else
            return inst;
    }
    else
    {
        if (immediate)
            dbg_printf("[%s], #%s%d", tbl_regs[get_nibble(inst, 4)], direction ? "" : "-", offset);
        else if (((inst >> 4) & 0xff) == 0x00) /* no shift */
            dbg_printf("[%s], %s", tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)]);
        else if (((inst >> 4) & 0x01) == 0x00) /* immediate shift (there's no register shift) */
            dbg_printf("[%s], %s, %s #%d", tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)],
                       tbl_shifts[(inst >> 5) & 0x03], (inst >> 7) & 0x1f);
        else
            return inst;
    }
    return 0;
}

static UINT arm_disasm_blocktrans(UINT inst, ADDRESS64 *addr)
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

    dbg_printf("\n\t%s%s%s\t%s%s, {", load ? "ldm" : "stm", tbl_addrmode[addrmode], get_cond(inst),
               tbl_regs[get_nibble(inst, 4)], writeback ? "!" : "");
    for (i=0;i<=15;i++)
        if ((inst>>i) & 1)
        {
            if (i == last) dbg_printf("%s", tbl_regs[i]);
            else dbg_printf("%s, ", tbl_regs[i]);
        }
    dbg_printf("}%s", psr ? "^" : "");
    return 0;
}

static UINT arm_disasm_swi(UINT inst, ADDRESS64 *addr)
{
    dbg_printf("\n\tswi%s\t#%d", get_cond(inst), inst & 0x00ffffff);
    return 0;
}

static UINT arm_disasm_coproctrans(UINT inst, ADDRESS64 *addr)
{
    WORD CRm    = inst & 0x0f;
    WORD CP     = (inst >> 5)  & 0x07;
    WORD CPnum  = (inst >> 8)  & 0x0f;
    WORD CRn    = (inst >> 16) & 0x0f;
    WORD load   = (inst >> 20) & 0x01;
    WORD CP_Opc = (inst >> 21) & 0x07;

    dbg_printf("\n\t%s%s\t%u, %u, %s, cr%u, cr%u, {%u}", load ? "mrc" : "mcr", get_cond(inst), CPnum,
               CP, tbl_regs[get_nibble(inst, 3)], CRn, CRm, CP_Opc);
    return 0;
}

static UINT arm_disasm_coprocdataop(UINT inst, ADDRESS64 *addr)
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

static UINT arm_disasm_coprocdatatrans(UINT inst, ADDRESS64 *addr)
{
    WORD CPnum  = (inst >> 8)  & 0x0f;
    WORD CRd    = (inst >> 12) & 0x0f;
    WORD load      = (inst >> 20) & 0x01;
    WORD writeback = (inst >> 21) & 0x01;
    WORD translen  = (inst >> 22) & 0x01;
    WORD direction = (inst >> 23) & 0x01;
    WORD indexing  = (inst >> 24) & 0x01;
    short offset    = (inst & 0xff) << 2;

    dbg_printf("\n\t%s%s%s", load ? "ldc" : "stc", translen ? "l" : "", get_cond(inst));
    if (indexing)
        dbg_printf("\t%u, cr%u, [%s, #%s%d]%s", CPnum, CRd, tbl_regs[get_nibble(inst, 4)], direction ? "" : "-", offset, writeback?"!":"");
    else
        dbg_printf("\t%u, cr%u, [%s], #%s%d", CPnum, CRd, tbl_regs[get_nibble(inst, 4)], direction ? "" : "-", offset);
    return 0;
}

static WORD thumb_disasm_hireg(WORD inst, ADDRESS64 *addr)
{
    short dst = inst & 0x07;
    short src = (inst >> 3) & 0x07;
    short h2  = (inst >> 6) & 0x01;
    short h1  = (inst >> 7) & 0x01;
    short op  = (inst >> 8) & 0x03;

    if (h1) dst += 8;
    if (h2) src += 8;

    if (op == 2 && dst == src) /* mov rx, rx */
    {
        dbg_printf("\n\tnop");
        return 0;
    }

    if (op == 3)
        dbg_printf("\n\tb%sx\t%s", h1?"l":"", tbl_regs[src]);
    else
        dbg_printf("\n\t%s\t%s, %s", tbl_hiops_t[op], tbl_regs[dst], tbl_regs[src]);

    return 0;
}

static WORD thumb_disasm_aluop(WORD inst, ADDRESS64 *addr)
{
    short dst = inst & 0x07;
    short src = (inst >> 3) & 0x07;
    short op  = (inst >> 6) & 0x0f;

    dbg_printf("\n\t%s\t%s, %s", tbl_aluops_t[op], tbl_regs[dst], tbl_regs[src]);

    return 0;
}

static WORD thumb_disasm_pushpop(WORD inst, ADDRESS64 *addr)
{
    short lrpc = (inst >> 8)  & 0x01;
    short load = (inst >> 11) & 0x01;
    short i;
    short last;

    for (i=7;i>=0;i--)
        if ((inst>>i) & 1) break;
    last = i;

    dbg_printf("\n\t%s\t{", load ? "pop" : "push");

    for (i=0;i<=7;i++)
        if ((inst>>i) & 1)
        {
            if (i == last) dbg_printf("%s", tbl_regs[i]);
            else dbg_printf("%s, ", tbl_regs[i]);
        }
    if (lrpc)
        dbg_printf("%s%s", last ? ", " : "", load ? "pc" : "lr");

    dbg_printf("}");
    return 0;
}

static WORD thumb_disasm_blocktrans(WORD inst, ADDRESS64 *addr)
{
    short load = (inst >> 11) & 0x01;
    short i;
    short last;

    for (i=7;i>=0;i--)
        if ((inst>>i) & 1) break;
    last = i;

    dbg_printf("\n\t%s\t%s!, {", load ? "ldmia" : "stmia", tbl_regs[(inst >> 8) & 0x07]);

    for (i=0;i<=7;i++)
        if ((inst>>i) & 1)
        {
            if (i == last) dbg_printf("%s", tbl_regs[i]);
            else dbg_printf("%s, ", tbl_regs[i]);
        }

    dbg_printf("}");
    return 0;
}

static WORD thumb_disasm_swi(WORD inst, ADDRESS64 *addr)
{
    dbg_printf("\n\tswi\t#%d", inst & 0x00ff);
    return 0;
}

static WORD thumb_disasm_condbranch(WORD inst, ADDRESS64 *addr)
{
    WORD offset = inst & 0x00ff;
    dbg_printf("\n\tb%s\t", tbl_cond[(inst >> 8) & 0x0f]);
    db_printsym(addr->Offset + offset);
    return 0;
}

static WORD thumb_disasm_uncondbranch(WORD inst, ADDRESS64 *addr)
{
    short offset = (inst & 0x07ff) << 1;

    if (offset & 0x0800) offset |= 0xf000;
    offset += 4;

    dbg_printf("\n\tb\t");
    db_printsym(addr->Offset + offset);
    return 0;
}

static WORD thumb_disasm_loadadr(WORD inst, ADDRESS64 *addr)
{
    WORD src = (inst >> 11) & 0x01;
    WORD offset = (inst & 0xff) << 2;

    dbg_printf("\n\tadd\t%s, %s, #%d", tbl_regs[(inst >> 8) & 0x07], src ? "sp" : "pc", offset);
    return 0;
}

static WORD thumb_disasm_ldrpcrel(WORD inst, ADDRESS64 *addr)
{
    WORD offset = (inst & 0xff) << 2;
    dbg_printf("\n\tldr\t%s, [pc, #%u]", tbl_regs[(inst >> 8) & 0x07], offset);
    return 0;
}

static WORD thumb_disasm_ldrsprel(WORD inst, ADDRESS64 *addr)
{
    WORD offset = (inst & 0xff) << 2;
    dbg_printf("\n\t%s\t%s, [sp, #%u]", (inst & 0x0800)?"ldr":"str", tbl_regs[(inst >> 8) & 0x07], offset);
    return 0;
}

static WORD thumb_disasm_addsprel(WORD inst, ADDRESS64 *addr)
{
    WORD offset = (inst & 0x7f) << 2;
    if ((inst >> 7) & 0x01)
        dbg_printf("\n\tsub\tsp, sp, #%u", offset);
    else
        dbg_printf("\n\tadd\tsp, sp, #%u", offset);
    return 0;
}

static WORD thumb_disasm_ldrimm(WORD inst, ADDRESS64 *addr)
{
    WORD offset = (inst & 0x07c0) >> 6;
    dbg_printf("\n\t%s%s\t%s, [%s, #%u]", (inst & 0x0800)?"ldr":"str", (inst & 0x1000)?"b":"",
               tbl_regs[inst & 0x07], tbl_regs[(inst >> 3) & 0x07], (inst & 0x1000)?offset:(offset << 2));
    return 0;
}

static WORD thumb_disasm_ldrhimm(WORD inst, ADDRESS64 *addr)
{
    WORD offset = (inst & 0x07c0) >> 5;
    dbg_printf("\n\t%s\t%s, [%s, #%u]", (inst & 0x0800)?"ldrh":"strh",
               tbl_regs[inst & 0x07], tbl_regs[(inst >> 3) & 0x07], offset);
    return 0;
}

static WORD thumb_disasm_ldrreg(WORD inst, ADDRESS64 *addr)
{
    dbg_printf("\n\t%s%s\t%s, [%s, %s]", (inst & 0x0800)?"ldr":"str", (inst & 0x0400)?"b":"",
               tbl_regs[inst & 0x07], tbl_regs[(inst >> 3) & 0x07], tbl_regs[(inst >> 6) & 0x07]);
    return 0;
}

static WORD thumb_disasm_ldrsreg(WORD inst, ADDRESS64 *addr)
{
    dbg_printf("\n\t%s\t%s, [%s, %s]", tbl_sregops_t[(inst >> 10) & 0x03],
               tbl_regs[inst & 0x07], tbl_regs[(inst >> 3) & 0x07], tbl_regs[(inst >> 6) & 0x07]);
    return 0;
}

static WORD thumb_disasm_immop(WORD inst, ADDRESS64 *addr)
{
    WORD op = (inst >> 11) & 0x03;
    dbg_printf("\n\t%s\t%s, #%u", tbl_immops_t[op], tbl_regs[(inst >> 8) & 0x07], inst & 0xff);
    return 0;
}

static WORD thumb_disasm_nop(WORD inst, ADDRESS64 *addr)
{
    dbg_printf("\n\tnop");
    return 0;
}

static WORD thumb_disasm_addsub(WORD inst, ADDRESS64 *addr)
{
    WORD op = (inst >> 9) & 0x01;
    WORD immediate = (inst >> 10) & 0x01;

    dbg_printf("\n\t%s\t%s, %s, ", op ? "sub" : "add",
               tbl_regs[inst & 0x07], tbl_regs[(inst >> 3) & 0x07]);
    if (immediate)
        dbg_printf("#%d", (inst >> 6) & 0x07);
    else
        dbg_printf("%s", tbl_regs[(inst >> 6) & 0x07]);
    return 0;
}

static WORD thumb_disasm_movshift(WORD inst, ADDRESS64 *addr)
{
    WORD op = (inst >> 11) & 0x03;
    dbg_printf("\n\t%s\t%s, %s, #%u", tbl_shifts[op],
               tbl_regs[inst & 0x07], tbl_regs[(inst >> 3) & 0x07], (inst >> 6) & 0x1f);
    return 0;
}

static UINT thumb2_disasm_srtrans(UINT inst, ADDRESS64 *addr)
{
    UINT fromsr = (inst >> 21) & 0x03;
    UINT sysreg = inst & 0xff;

    if (fromsr == 3 && get_nibble(inst,4) == 0x0f && sysreg <= 20)
    {
        dbg_printf("\n\tmrs\t%s, %s", tbl_regs[get_nibble(inst, 2)], tbl_special_regs_t2[sysreg]);
        return 0;
    }

    if (fromsr == 0 && sysreg <= 20)
    {
        dbg_printf("\n\tmsr\t%s, %s", tbl_special_regs_t2[sysreg], tbl_regs[get_nibble(inst, 4)]);
        return 0;
    }

    return inst;
}

static UINT thumb2_disasm_hint(UINT inst, ADDRESS64 *addr)
{
    WORD op1 = (inst >> 8) & 0x07;
    WORD op2 = inst & 0xff;

    if (op1) return inst;

    if (op2 <= 4)
    {
        dbg_printf("\n\t%s", tbl_hints_t2[op2]);
        return 0;
    }

    if (op2 & 0xf0)
    {
        dbg_printf("\n\tdbg\t#%u", get_nibble(inst, 0));
        return 0;
    }

    return inst;
}

static UINT thumb2_disasm_miscctrl(UINT inst, ADDRESS64 *addr)
{
    WORD op = (inst >> 4) & 0x0f;

    switch (op)
    {
    case 2:
        dbg_printf("\n\tclrex");
        break;
    case 4:
        dbg_printf("\n\tdsb\t#%u", get_nibble(inst, 0));
        break;
    case 5:
        dbg_printf("\n\tdmb\t#%u", get_nibble(inst, 0));
        break;
    case 6:
        dbg_printf("\n\tisb\t#%u", get_nibble(inst, 0));
        break;
    default:
        return inst;
    }

    return 0;
}

static UINT thumb2_disasm_branch(UINT inst, ADDRESS64 *addr)
{
    UINT S  = (inst >> 26) & 0x01;
    UINT L  = (inst >> 14) & 0x01;
    UINT I1 = !(((inst >> 13) & 0x01) ^ S);
    UINT C  = !((inst >> 12) & 0x01);
    UINT I2 = !(((inst >> 11) & 0x01) ^ S);
    UINT offset = (inst & 0x000007ff) << 1;

    if (C)
    {
        offset |= I1 << 19 | I2 << 18 | (inst & 0x003f0000) >> 4;
        if (S) offset |= 0x0fff << 20;
    }
    else
    {
        offset |= I1 << 23 | I2 << 22 | (inst & 0x03ff0000) >> 4;
        if (S) offset |= 0xff << 24;
    }

    dbg_printf("\n\tb%s%s\t", L ? "l" : "", C ? tbl_cond[(inst >> 22) & 0x0f] : "");
    db_printsym(addr->Offset + offset + 4);
    return 0;
}

static UINT thumb2_disasm_misc(UINT inst, ADDRESS64 *addr)
{
    WORD op1 = (inst >> 20) & 0x03;
    WORD op2 = (inst >> 4) & 0x03;

    if (get_nibble(inst, 4) != get_nibble(inst, 0))
        return inst;

    if (op1 == 3 && op2 == 0)
    {
        dbg_printf("\n\tclz\t%s, %s", tbl_regs[get_nibble(inst, 2)], tbl_regs[get_nibble(inst, 0)]);
        return 0;
    }

    if (op1 == 1)
    {
        dbg_printf("\n\t%s\t%s, %s", tbl_miscops_t2[op2], tbl_regs[get_nibble(inst, 2)],
                   tbl_regs[get_nibble(inst, 0)]);
        return 0;
    }

    return inst;
}

static UINT thumb2_disasm_dataprocessingreg(UINT inst, ADDRESS64 *addr)
{
    WORD op1 = (inst >> 20) & 0x07;
    WORD op2 = (inst >> 4) & 0x0f;

    if (!op2)
    {
        dbg_printf("\n\t%s%s\t%s, %s, %s", tbl_shifts[op1 >> 1], (op1 & 1)?"s":"",
                   tbl_regs[get_nibble(inst, 2)], tbl_regs[get_nibble(inst, 4)],
                   tbl_regs[get_nibble(inst, 0)]);
        return 0;
    }

    if ((op2 & 0x0C) == 0x08 && get_nibble(inst, 4) == 0x0f)
    {
        dbg_printf("\n\t%sxt%s\t%s, %s", (op1 & 1)?"u":"s", (op1 & 4)?"b":"h",
                   tbl_regs[get_nibble(inst, 2)], tbl_regs[get_nibble(inst, 0)]);
        if (op2 & 0x03)
            dbg_printf(", ROR #%u", (op2 & 3) * 8);
        return 0;
    }

    return inst;
}

static UINT thumb2_disasm_mul(UINT inst, ADDRESS64 *addr)
{
    WORD op1 = (inst >> 20) & 0x07;
    WORD op2 = (inst >> 4) & 0x03;

    if (op1)
        return inst;

    if (op2 == 0 && get_nibble(inst, 3) != 0xf)
    {
        dbg_printf("\n\tmla\t%s, %s, %s, %s", tbl_regs[get_nibble(inst, 2)],
                                                tbl_regs[get_nibble(inst, 4)],
                                                tbl_regs[get_nibble(inst, 0)],
                                                tbl_regs[get_nibble(inst, 3)]);
        return 0;
    }

    if (op2 == 0 && get_nibble(inst, 3) == 0xf)
    {
        dbg_printf("\n\tmul\t%s, %s, %s", tbl_regs[get_nibble(inst, 2)],
                                            tbl_regs[get_nibble(inst, 4)],
                                            tbl_regs[get_nibble(inst, 0)]);
        return 0;
    }

    if (op2 == 1)
    {
        dbg_printf("\n\tmls\t%s, %s, %s, %s", tbl_regs[get_nibble(inst, 2)],
                                                tbl_regs[get_nibble(inst, 4)],
                                                tbl_regs[get_nibble(inst, 0)],
                                                tbl_regs[get_nibble(inst, 3)]);
        return 0;
    }

    return inst;
}

static UINT thumb2_disasm_longmuldiv(UINT inst, ADDRESS64 *addr)
{
    WORD op1 = (inst >> 20) & 0x07;
    WORD op2 = (inst >> 4) & 0x0f;

    if (op2 == 0)
    {
        switch (op1)
        {
        case 0:
            dbg_printf("\n\tsmull\t");
            break;
        case 2:
            dbg_printf("\n\tumull\t");
            break;
        case 4:
            dbg_printf("\n\tsmlal\t");
            break;
        case 6:
            dbg_printf("\n\tumlal\t");
            break;
        default:
            return inst;
        }
        dbg_printf("%s, %s, %s, %s", tbl_regs[get_nibble(inst, 3)], tbl_regs[get_nibble(inst, 2)],
                                       tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)]);
        return 0;
    }

    if (op2 == 0xffff)
    {
        switch (op1)
        {
        case 1:
            dbg_printf("\n\tsdiv\t");
            break;
        case 3:
            dbg_printf("\n\tudiv\t");
            break;
        default:
            return inst;
        }
        dbg_printf("%s, %s, %s", tbl_regs[get_nibble(inst, 2)], tbl_regs[get_nibble(inst, 4)],
                                   tbl_regs[get_nibble(inst, 0)]);
        return 0;
    }

    return inst;
}

static UINT thumb2_disasm_str(UINT inst, ADDRESS64 *addr)
{
    WORD op1 = (inst >> 21) & 0x07;
    WORD op2 = (inst >> 6) & 0x3f;

    if ((op1 & 0x03) == 3) return inst;

    if (!(op1 & 0x04) && inst & 0x0800)
    {
        int offset;
        dbg_printf("\n\tstr%s\t%s, [%s", tbl_width_t2[op1 & 0x03], tbl_regs[get_nibble(inst, 3)],
                   tbl_regs[get_nibble(inst, 4)]);

        offset = inst & 0xff;
        if (!(inst & 0x0200)) offset *= -1;

        if (!(inst & 0x0400) && (inst & 0x0100)) dbg_printf("], #%i", offset);
        else if (inst & 0x0400) dbg_printf(", #%i]%s", offset, (inst & 0x0100)?"!":"");
        else return inst;
        return 0;
    }

    if (!(op1 & 0x04) && !op2)
    {
        dbg_printf("\n\tstr%s\t%s, [%s, %s, LSL #%u]", tbl_width_t2[op1 & 0x03],
                   tbl_regs[get_nibble(inst, 3)], tbl_regs[get_nibble(inst, 4)],
                   tbl_regs[get_nibble(inst, 0)], (inst >> 4) & 0x3);
        return 0;
    }

    if (op1 & 0x04)
    {
        dbg_printf("\n\tstr%s\t%s, [%s, #%u]", tbl_width_t2[op1 & 0x03],
                   tbl_regs[get_nibble(inst, 3)], tbl_regs[get_nibble(inst, 4)], inst & 0x0fff);
        return 0;
    }

    return inst;
}

static UINT thumb2_disasm_ldrword(UINT inst, ADDRESS64 *addr)
{
    WORD op1 = (inst >> 23) & 0x01;
    WORD op2 = (inst >> 6) & 0x3f;
    int offset;

    if (get_nibble(inst, 4) == 0x0f)
    {
        offset = inst & 0x0fff;

        if (!op1) offset *= -1;
        offset += 3;

        dbg_printf("\n\tldr\t%s, ", tbl_regs[get_nibble(inst, 3)]);
        db_printsym(addr->Offset + offset);
        return 0;
    }

    if (!op1 && !op2)
    {
        dbg_printf("\n\tldr\t%s, [%s, %s, LSL #%u]", tbl_regs[get_nibble(inst, 3)],
                   tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)], (inst >> 4) & 0x3);
        return 0;
    }

    if (!op1 && (op2 & 0x3c) == 0x38)
    {
        dbg_printf("\n\tldrt\t%s, [%s, #%u]", tbl_regs[get_nibble(inst, 3)],
                   tbl_regs[get_nibble(inst, 4)], inst & 0xff);
        return 0;
    }

    dbg_printf("\n\tldr\t%s, [%s", tbl_regs[get_nibble(inst, 3)], tbl_regs[get_nibble(inst, 4)]);

    if (op1)
    {
        dbg_printf(", #%u]", inst & 0x0fff);
        return 0;
    }

    offset = inst & 0xff;
    if (!(inst & 0x0200)) offset *= -1;

    if (!(inst & 0x0400) && (inst & 0x0100)) dbg_printf("], #%i", offset);
    else if (inst & 0x0400) dbg_printf(", #%i]%s", offset, (inst & 0x0100)?"!":"");
    else return inst;

    return 0;
}

static UINT thumb2_disasm_preload(UINT inst, ADDRESS64 *addr)
{
    WORD op1 = (inst >> 23) & 0x03;

    if (!(op1 & 0x01) && !((inst >> 6) & 0x3f) && get_nibble(inst, 4) != 15)
    {
        WORD shift = (inst >> 4) & 0x03;
        dbg_printf("\n\t%s\t[%s, %s", op1?"pli":"pld", tbl_regs[get_nibble(inst, 4)],
                   tbl_regs[get_nibble(inst, 0)]);
        if (shift) dbg_printf(", lsl #%u]", shift);
        else dbg_printf("]");
        return 0;
    }

    if (get_nibble(inst, 4) != 15)
    {
        dbg_printf("\n\t%s\t[%s, #%d]", (op1 & 0x02)?"pli":"pld", tbl_regs[get_nibble(inst, 4)],
                   (op1 & 0x01)?(inst & 0x0fff):(-1 * (inst & 0xff)));
        return 0;
    }

    if (get_nibble(inst, 4) == 15)
    {
        int offset = inst & 0x0fff;
        if (!op1) offset *= -1;
        dbg_printf("\n\t%s\t", (op1 & 0x02)?"pli":"pld");
        db_printsym(addr->Offset + offset + 4);
        return 0;
    }

    return inst;
}

static UINT thumb2_disasm_ldrnonword(UINT inst, ADDRESS64 *addr)
{
    WORD op1 = (inst >> 23) & 0x03;
    WORD hw  = (inst >> 21) & 0x01;

    if (!(op1 & 0x01) && !((inst >> 6) & 0x3f) && get_nibble(inst, 4) != 15)
    {
        WORD shift = (inst >> 4) & 0x03;
        dbg_printf("\n\t%s%s\t%s, [%s, %s", op1?"ldrs":"ldr", hw?"h":"b",
                   tbl_regs[get_nibble(inst, 3)], tbl_regs[get_nibble(inst, 4)],
                   tbl_regs[get_nibble(inst, 0)]);
        if (shift) dbg_printf(", lsl #%u]", shift);
        else dbg_printf("]");
        return 0;
    }

    if (!(op1 & 0x01) && ((inst >> 8) & 0x0f) == 14 && get_nibble(inst, 4) != 15)
    {
        WORD offset = inst & 0xff;
        dbg_printf("\n\t%s%s\t%s, [%s", op1?"ldrs":"ldr", hw?"ht":"bt",
                   tbl_regs[get_nibble(inst, 3)], tbl_regs[get_nibble(inst, 4)]);
        if (offset) dbg_printf(", #%u]", offset);
        else dbg_printf("]");
        return 0;
    }

    if (get_nibble(inst, 4) != 15)
    {
        int offset;

        dbg_printf("\n\t%s%s\t%s, [%s", (op1 & 0x02)?"ldrs":"ldr", hw?"h":"b",
                   tbl_regs[get_nibble(inst, 3)], tbl_regs[get_nibble(inst, 4)]);

        if (op1 & 0x01)
        {
            dbg_printf(", #%u]", inst & 0x0fff);
            return 0;
        }

        offset = inst & 0xff;
        if (!(inst & 0x0200)) offset *= -1;

        if (!(inst & 0x0400) && (inst & 0x0100)) dbg_printf("], #%i", offset);
        else if (inst & 0x0400) dbg_printf(", #%i]%s", offset, (inst & 0x0100)?"!":"");
        else return inst;

        return 0;
    }

    if (get_nibble(inst, 4) == 15)
    {
        int offset = inst & 0x0fff;
        if (!op1) offset *= -1;
        dbg_printf("\n\t%s%s\t%s, ", (op1 & 0x02)?"ldrs":"ldr", hw?"h":"b",
                   tbl_regs[get_nibble(inst, 3)]);
        db_printsym(addr->Offset + offset + 4);
        return 0;
    }

    return inst;
}

static UINT thumb2_disasm_dataprocessing(UINT inst, ADDRESS64 *addr)
{
    WORD op = (inst >> 20) & 0x1f;
    WORD imm5 = ((inst >> 10) & 0x1c) + ((inst >> 6) & 0x03);

    switch (op)
    {
    case 0:
    {
        WORD offset = ((inst >> 15) & 0x0800) + ((inst >> 4) & 0x0700) + (inst & 0xff);
        if (get_nibble(inst, 4) == 15)
        {
            dbg_printf("\n\tadr\t%s, ", tbl_regs[get_nibble(inst, 2)]);
            db_printsym(addr->Offset + offset + 4);
        }
        else
            dbg_printf("\n\taddw\t%s, %s, #%u", tbl_regs[get_nibble(inst, 2)],
                       tbl_regs[get_nibble(inst, 4)], offset);
        return 0;
    }
    case 4:
    case 12:
    {
        WORD offset = ((inst >> 15) & 0x0800) + ((inst >> 4) & 0xf000) +
                      ((inst >>  4) & 0x0700) + (inst & 0xff);
        dbg_printf("\n\t%s\t%s, #%u", op == 12 ? "movt" : "movw", tbl_regs[get_nibble(inst, 2)],
                   offset);
        return 0;
    }
    case 10:
    {
        int offset = ((inst >> 15) & 0x0800) + ((inst >> 4) & 0x0700) + (inst & 0xff);
        if (get_nibble(inst, 4) == 15)
        {
            offset *= -1;
            dbg_printf("\n\tadr\t%s, ", tbl_regs[get_nibble(inst, 2)]);
            db_printsym(addr->Offset + offset + 4);
        }
        else
            dbg_printf("\n\tsubw\t%s, %s, #%u", tbl_regs[get_nibble(inst, 2)],
                       tbl_regs[get_nibble(inst, 4)], offset);
        return 0;
    }
    case 16:
    case 18:
    case 24:
    case 26:
    {
        BOOL sign = op < 24;
        WORD sh = (inst >> 21) & 0x01;
        WORD sat = (inst & 0x1f);
        if (sign) sat++;
        if (imm5)
            dbg_printf("\n\t%s\t%s, #%u, %s, %s #%u", sign ? "ssat" : "usat",
                       tbl_regs[get_nibble(inst, 2)], sat, tbl_regs[get_nibble(inst, 4)],
                       sh ? "asr" : "lsl", imm5);
        else
            dbg_printf("\n\t%s\t%s, #%u, %s", sign ? "ssat" : "usat", tbl_regs[get_nibble(inst, 2)],
                       sat, tbl_regs[get_nibble(inst, 4)]);
        return 0;
    }
    case 20:
    case 28:
    {
        WORD width = (inst & 0x1f) + 1;
        dbg_printf("\n\t%s\t%s, %s, #%u, #%u", op == 28 ? "ubfx" : "sbfx",
                   tbl_regs[get_nibble(inst, 2)], tbl_regs[get_nibble(inst, 4)], imm5, width);
        return 0;
    }
    case 22:
    {
        WORD msb = (inst & 0x1f) + 1 - imm5;
        if (get_nibble(inst, 4) == 15)
            dbg_printf("\n\tbfc\t%s, #%u, #%u", tbl_regs[get_nibble(inst, 2)], imm5, msb);
        else
            dbg_printf("\n\tbfi\t%s, %s, #%u, #%u", tbl_regs[get_nibble(inst, 2)],
                       tbl_regs[get_nibble(inst, 4)], imm5, msb);
        return 0;
    }
    default:
        return inst;
    }
}

static UINT thumb2_disasm_dataprocessingmod(UINT inst, ADDRESS64 *addr)
{
    WORD op = (inst >> 21) & 0x0f;
    WORD sf = (inst >> 20) & 0x01;
    WORD offset = ((inst >> 15) & 0x0800) + ((inst >> 4) & 0x0700) + (inst & 0xff);

    /* FIXME: use ThumbExpandImm_C */

    switch (op)
    {
    case 0:
        if (get_nibble(inst, 2) == 15)
            dbg_printf("\n\ttst\t%s, #%u", tbl_regs[get_nibble(inst, 4)], offset);
        else
            dbg_printf("\n\tand%s\t%s, %s, #%u", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)],
                       tbl_regs[get_nibble(inst, 4)], offset);
        return 0;
    case 1:
        dbg_printf("\n\tbic%s\t%s, %s, #%u", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)],
                   tbl_regs[get_nibble(inst, 4)], offset);
        return 0;
    case 2:
        if (get_nibble(inst, 4) == 15)
            dbg_printf("\n\tmov%s\t%s, #%u", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)], offset);
        else
            dbg_printf("\n\torr%s\t%s, %s, #%u", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)],
                       tbl_regs[get_nibble(inst, 4)], offset);
        return 0;
    case 3:
        if (get_nibble(inst, 4) == 15)
            dbg_printf("\n\tmvn%s\t%s, #%u", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)], offset);
        else
            dbg_printf("\n\torn%s\t%s, %s, #%u", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)],
                       tbl_regs[get_nibble(inst, 4)], offset);
        return 0;
    case 4:
        if (get_nibble(inst, 2) == 15)
            dbg_printf("\n\tteq\t%s, #%u", tbl_regs[get_nibble(inst, 4)], offset);
        else
            dbg_printf("\n\teor%s\t%s, %s, #%u", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)],
                       tbl_regs[get_nibble(inst, 4)], offset);
        return 0;
    case 8:
        if (get_nibble(inst, 2) == 15)
            dbg_printf("\n\tcmn\t%s, #%u", tbl_regs[get_nibble(inst, 4)], offset);
        else
            dbg_printf("\n\tadd%s\t%s, %s, #%u", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)],
                       tbl_regs[get_nibble(inst, 4)], offset);
        return 0;
    case 10:
        dbg_printf("\n\tadc%s\t%s, %s, #%u", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)],
                   tbl_regs[get_nibble(inst, 4)], offset);
        return 0;
    case 11:
        dbg_printf("\n\tsbc%s\t%s, %s, #%u", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)],
                   tbl_regs[get_nibble(inst, 4)], offset);
        return 0;
    case 13:
        if (get_nibble(inst, 2) == 15)
            dbg_printf("\n\tcmp\t%s, #%u", tbl_regs[get_nibble(inst, 4)], offset);
        else
            dbg_printf("\n\tsub%s\t%s, %s, #%u", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)],
                       tbl_regs[get_nibble(inst, 4)], offset);
        return 0;
    case 14:
        dbg_printf("\n\trsb%s\t%s, %s, #%u", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)],
                   tbl_regs[get_nibble(inst, 4)], offset);
        return 0;
    default:
        return inst;
    }
}

static UINT thumb2_disasm_dataprocessingshift(UINT inst, ADDRESS64 *addr)
{
    WORD op = (inst >> 21) & 0x0f;
    WORD sf = (inst >> 20) & 0x01;
    WORD imm5 = ((inst >> 10) & 0x1c) + ((inst >> 6) & 0x03);
    WORD type = (inst >> 4) & 0x03;

    if (!imm5 && (type == 1 || type == 2)) imm5 = 32;
    else if (!imm5 && type == 3) type = 4;

    switch (op)
    {
    case 0:
        if (get_nibble(inst, 2) == 15)
            dbg_printf("\n\ttst\t%s, %s", tbl_regs[get_nibble(inst, 4)],
                       tbl_regs[get_nibble(inst, 0)]);
        else
            dbg_printf("\n\tand%s\t%s, %s, %s", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)],
                       tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)]);
        break;
    case 1:
        dbg_printf("\n\tbic%s\t%s, %s, %s", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)],
                   tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)]);
        break;
    case 2:
        if (get_nibble(inst, 4) == 15)
        {
            if (type == 4)
                dbg_printf("\n\trrx%s\t%s, %s", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)], tbl_regs[get_nibble(inst, 0)]);
            else if (!type && !imm5)
                dbg_printf("\n\tmov%s\t%s, %s", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)], tbl_regs[get_nibble(inst, 0)]);
            else
                dbg_printf("\n\t%s%s\t%s, %s, #%u", tbl_shifts[type], sf ? "s" : "", tbl_regs[get_nibble(inst, 2)], tbl_regs[get_nibble(inst, 0)], imm5);
            return 0;
        }
        else
            dbg_printf("\n\torr%s\t%s, %s, %s", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)],
                       tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)]);
        break;
    case 3:
        if (get_nibble(inst, 4) == 15)
            dbg_printf("\n\tmvn%s\t%s, %s, %s", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)],
                       tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)]);
        else
            dbg_printf("\n\torn%s\t%s, %s, %s", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)],
                       tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)]);
        break;
    case 4:
        if (get_nibble(inst, 2) == 15)
            dbg_printf("\n\tteq\t%s, %s", tbl_regs[get_nibble(inst, 4)],
                       tbl_regs[get_nibble(inst, 0)]);
        else
            dbg_printf("\n\teor%s\t%s, %s, %s", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)],
                       tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)]);
        break;
    case 8:
        if (get_nibble(inst, 2) == 15)
            dbg_printf("\n\tcmn\t%s, %s", tbl_regs[get_nibble(inst, 4)],
                       tbl_regs[get_nibble(inst, 0)]);
        else
            dbg_printf("\n\tadd%s\t%s, %s, %s", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)],
                       tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)]);
        break;
    case 10:
        dbg_printf("\n\tadc%s\t%s, %s, %s", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)],
                   tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)]);
        break;
    case 11:
        dbg_printf("\n\tsbc%s\t%s, %s, %s", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)],
                   tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)]);
        break;
    case 13:
        if (get_nibble(inst, 2) == 15)
            dbg_printf("\n\tcmp\t%s, %s", tbl_regs[get_nibble(inst, 4)],
                       tbl_regs[get_nibble(inst, 0)]);
        else
            dbg_printf("\n\tsub%s\t%s, %s, %s", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)],
                       tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)]);
        break;
    case 14:
        dbg_printf("\n\trsb%s\t%s, %s, %s", sf ? "s" : "", tbl_regs[get_nibble(inst, 2)],
                   tbl_regs[get_nibble(inst, 4)], tbl_regs[get_nibble(inst, 0)]);
        break;
    default:
        return inst;
    }

    if (type == 4)
        dbg_printf(", rrx");
    else if (type || imm5)
        dbg_printf(", %s #%u", tbl_shifts[type], imm5);
    return 0;
}

static UINT thumb2_disasm_coprocdat(UINT inst, ADDRESS64 *addr)
{
    WORD opc2 = (inst >> 5) & 0x07;

    dbg_printf("\n\tcdp%s\tp%u, #%u, cr%u, cr%u, cr%u", (inst & 0x10000000)?"2":"",
               get_nibble(inst, 2), get_nibble(inst, 5), get_nibble(inst, 3),
               get_nibble(inst, 4), get_nibble(inst, 0));

    if (opc2) dbg_printf(", #%u", opc2);
    return 0;
}

static UINT thumb2_disasm_coprocmov1(UINT inst, ADDRESS64 *addr)
{
    WORD opc1 = (inst >> 21) & 0x07;
    WORD opc2 = (inst >> 5) & 0x07;

    dbg_printf("\n\t%s%s\tp%u, #%u, %s, cr%u, cr%u", (inst & 0x00100000)?"mrc":"mcr",
               (inst & 0x10000000)?"2":"", get_nibble(inst, 2), opc1,
               tbl_regs[get_nibble(inst, 3)], get_nibble(inst, 4), get_nibble(inst, 0));

    if (opc2) dbg_printf(", #%u", opc2);
    return 0;
}

static UINT thumb2_disasm_coprocmov2(UINT inst, ADDRESS64 *addr)
{
    dbg_printf("\n\t%s%s\tp%u, #%u, %s, %s, cr%u", (inst & 0x00100000)?"mrrc":"mcrr",
               (inst & 0x10000000)?"2":"", get_nibble(inst, 2), get_nibble(inst, 1),
               tbl_regs[get_nibble(inst, 3)], tbl_regs[get_nibble(inst, 4)], get_nibble(inst, 0));

    return 0;
}

static UINT thumb2_disasm_coprocdatatrans(UINT inst, ADDRESS64 *addr)
{
    WORD indexing  = (inst >> 24) & 0x01;
    WORD direction = (inst >> 23) & 0x01;
    WORD translen  = (inst >> 22) & 0x01;
    WORD writeback = (inst >> 21) & 0x01;
    WORD load      = (inst >> 20) & 0x01;
    short offset    = (inst & 0xff) << 2;

    if (!direction) offset *= -1;

    dbg_printf("\n\t%s%s%s", load ? "ldc" : "stc", (inst & 0x10000000)?"2":"", translen ? "l" : "");
    if (indexing)
    {
        if (load && get_nibble(inst, 4) == 15)
        {
            dbg_printf("\tp%u, cr%u, ", get_nibble(inst, 2), get_nibble(inst, 3));
            db_printsym(addr->Offset + offset + 4);
        }
        else
            dbg_printf("\tp%u, cr%u, [%s, #%d]%s", get_nibble(inst, 2), get_nibble(inst, 3), tbl_regs[get_nibble(inst, 4)], offset, writeback?"!":"");
    }
    else
    {
        if (writeback)
            dbg_printf("\tp%u, cr%u, [%s], #%d", get_nibble(inst, 2), get_nibble(inst, 3), tbl_regs[get_nibble(inst, 4)], offset);
        else
            dbg_printf("\tp%u, cr%u, [%s], {%u}", get_nibble(inst, 2), get_nibble(inst, 3), tbl_regs[get_nibble(inst, 4)], inst & 0xff);
    }
    return 0;
}

static UINT thumb2_disasm_ldrstrmul(UINT inst, ADDRESS64 *addr)
{
    short load      = (inst >> 20) & 0x01;
    short writeback = (inst >> 21) & 0x01;
    short decbefore = (inst >> 24) & 0x01;
    short i;
    short last=15;
    for (i=15;i>=0;i--)
        if ((inst>>i) & 1)
        {
            last = i;
            break;
        }

    if (writeback && get_nibble(inst, 4) == 13)
        dbg_printf("\n\t%s\t{", load ? "pop" : "push");
    else
        dbg_printf("\n\t%s%s\t%s%s, {", load ? "ldm" : "stm", decbefore ? "db" : "ia",
                   tbl_regs[get_nibble(inst, 4)], writeback ? "!" : "");
    for (i=0;i<=15;i++)
        if ((inst>>i) & 1)
        {
            if (i == last) dbg_printf("%s", tbl_regs[i]);
            else dbg_printf("%s, ", tbl_regs[i]);
        }
    dbg_printf("}");
    return 0;
}

static UINT thumb2_disasm_ldrstrextbr(UINT inst, ADDRESS64 *addr)
{
    WORD op1 = (inst >> 23) & 0x03;
    WORD op2 = (inst >> 20) & 0x03;
    WORD op3 = (inst >>  4) & 0x0f;
    WORD indexing  = (inst >> 24) & 0x01;
    WORD direction = (inst >> 23) & 0x01;
    WORD writeback = (inst >> 21) & 0x01;
    WORD load      = (inst >> 20) & 0x01;
    short offset   = (inst & 0xff) << 2;

    if (op1 == 1 && op2 == 1 && op3 < 2)
    {
        WORD halfword = (inst >> 4) & 0x01;
        if (halfword)
            dbg_printf("\n\ttbh\t [%s, %s, lsl #1]", tbl_regs[get_nibble(inst, 4)],
                       tbl_regs[get_nibble(inst, 0)]);
        else
            dbg_printf("\n\ttbb\t [%s, %s]", tbl_regs[get_nibble(inst, 4)],
                       tbl_regs[get_nibble(inst, 0)]);
        return 0;
    }

    if (op1 == 0 && op2 < 2)
    {
        if (get_nibble(inst, 2) == 15)
            dbg_printf("\n\tldrex\t %s, [%s, #%u]", tbl_regs[get_nibble(inst, 3)],
                       tbl_regs[get_nibble(inst, 4)], offset);
        else
            dbg_printf("\n\tstrex\t %s, %s, [%s, #%u]", tbl_regs[get_nibble(inst, 2)],
                       tbl_regs[get_nibble(inst, 3)], tbl_regs[get_nibble(inst, 4)], offset);
        return 0;
    }

    if (op1 == 1 && op2 < 2)
    {
        WORD halfword = (inst >> 4) & 0x01;
        if (get_nibble(inst, 0) == 15)
            dbg_printf("\n\tldrex%s\t %s, [%s]", halfword ? "h" : "b",
                       tbl_regs[get_nibble(inst, 3)], tbl_regs[get_nibble(inst, 4)]);
        else
            dbg_printf("\n\tstrex%s\t %s, %s, [%s]", halfword ? "h" : "b",
                       tbl_regs[get_nibble(inst, 0)], tbl_regs[get_nibble(inst, 3)],
                       tbl_regs[get_nibble(inst, 4)]);
        return 0;
    }

    if (!direction) offset *= -1;
    dbg_printf("\n\t%s\t", load ? "ldrd" : "strd");
    if (indexing)
    {
        if (load && get_nibble(inst, 4) == 15)
        {
            dbg_printf("%s, %s, ", tbl_regs[get_nibble(inst, 3)], tbl_regs[get_nibble(inst, 2)]);
            db_printsym(addr->Offset + offset + 4);
        }
        else
            dbg_printf("%s, %s, [%s, #%d]%s", tbl_regs[get_nibble(inst, 3)],
                       tbl_regs[get_nibble(inst, 2)], tbl_regs[get_nibble(inst, 4)], offset,
                       writeback?"!":"");
    }
    else
        dbg_printf("%s, %s, [%s], #%d", tbl_regs[get_nibble(inst, 3)],
                   tbl_regs[get_nibble(inst, 2)], tbl_regs[get_nibble(inst, 4)], offset);
    return 0;
}

struct inst_arm
{
        UINT mask;
        UINT pattern;
        UINT (*func)(UINT, ADDRESS64*);
};

static const struct inst_arm tbl_arm[] = {
    { 0x0e000000, 0x0a000000, arm_disasm_branch },
    { 0x0fc000f0, 0x00000090, arm_disasm_mul },
    { 0x0f8000f0, 0x00800090, arm_disasm_longmul },
    { 0x0fb00ff0, 0x01000090, arm_disasm_swp },
    { 0x0e000090, 0x00000090, arm_disasm_halfwordtrans },
    { 0x0ffffff0, 0x012fff10, arm_disasm_branchxchg },
    { 0x0fbf0fff, 0x010f0000, arm_disasm_mrstrans },
    { 0x0dbef000, 0x0128f000, arm_disasm_msrtrans },
    { 0x0fb00000, 0x03000000, arm_disasm_wordmov },
    { 0x0fffffff, 0x0320f000, arm_disasm_nop },
    { 0x0c000000, 0x00000000, arm_disasm_dataprocessing },
    { 0x0c000000, 0x04000000, arm_disasm_singletrans },
    { 0x0e000000, 0x08000000, arm_disasm_blocktrans },
    { 0x0f000000, 0x0f000000, arm_disasm_swi },
    { 0x0f000010, 0x0e000010, arm_disasm_coproctrans },
    { 0x0f000010, 0x0e000000, arm_disasm_coprocdataop },
    { 0x0e000000, 0x0c000000, arm_disasm_coprocdatatrans },
    { 0x00000000, 0x00000000, NULL }
};

struct inst_thumb16
{
        WORD mask;
        WORD pattern;
        WORD (*func)(WORD, ADDRESS64*);
};

static const struct inst_thumb16 tbl_thumb16[] = {
    { 0xfc00, 0x4400, thumb_disasm_hireg },
    { 0xfc00, 0x4000, thumb_disasm_aluop },
    { 0xf600, 0xb400, thumb_disasm_pushpop },
    { 0xf000, 0xc000, thumb_disasm_blocktrans },
    { 0xff00, 0xdf00, thumb_disasm_swi },
    { 0xf000, 0xd000, thumb_disasm_condbranch },
    { 0xf800, 0xe000, thumb_disasm_uncondbranch },
    { 0xf000, 0xa000, thumb_disasm_loadadr },
    { 0xf800, 0x4800, thumb_disasm_ldrpcrel },
    { 0xf000, 0x9000, thumb_disasm_ldrsprel },
    { 0xff00, 0xb000, thumb_disasm_addsprel },
    { 0xe000, 0x6000, thumb_disasm_ldrimm },
    { 0xf000, 0x8000, thumb_disasm_ldrhimm },
    { 0xf200, 0x5000, thumb_disasm_ldrreg },
    { 0xf200, 0x5200, thumb_disasm_ldrsreg },
    { 0xe000, 0x2000, thumb_disasm_immop },
    { 0xff00, 0xbf00, thumb_disasm_nop },
    { 0xf800, 0x1800, thumb_disasm_addsub },
    { 0xe000, 0x0000, thumb_disasm_movshift },
    { 0x0000, 0x0000, NULL }
};

static const struct inst_arm tbl_thumb32[] = {
    { 0xfff0f000, 0xf3e08000, thumb2_disasm_srtrans },
    { 0xfff0f000, 0xf3808000, thumb2_disasm_srtrans },
    { 0xfff0d000, 0xf3a08000, thumb2_disasm_hint },
    { 0xfff0d000, 0xf3b08000, thumb2_disasm_miscctrl },
    { 0xf8008000, 0xf0008000, thumb2_disasm_branch },
    { 0xffc0f0c0, 0xfa80f080, thumb2_disasm_misc },
    { 0xff80f000, 0xfa00f000, thumb2_disasm_dataprocessingreg },
    { 0xff8000c0, 0xfb000000, thumb2_disasm_mul },
    { 0xff8000f0, 0xfb800000, thumb2_disasm_longmuldiv },
    { 0xff8000f0, 0xfb8000f0, thumb2_disasm_longmuldiv },
    { 0xff100000, 0xf8000000, thumb2_disasm_str },
    { 0xff700000, 0xf8500000, thumb2_disasm_ldrword },
    { 0xfe70f000, 0xf810f000, thumb2_disasm_preload },
    { 0xfe500000, 0xf8100000, thumb2_disasm_ldrnonword },
    { 0xfa008000, 0xf2000000, thumb2_disasm_dataprocessing },
    { 0xfa008000, 0xf0000000, thumb2_disasm_dataprocessingmod },
    { 0xfe008000, 0xea000000, thumb2_disasm_dataprocessingshift },
    { 0xef000010, 0xee000000, thumb2_disasm_coprocdat },
    { 0xef000010, 0xee000010, thumb2_disasm_coprocmov1 },
    { 0xefe00000, 0xec400000, thumb2_disasm_coprocmov2 },
    { 0xee000000, 0xec000000, thumb2_disasm_coprocdatatrans },
    { 0xfe402000, 0xe8000000, thumb2_disasm_ldrstrmul },
    { 0xfe400000, 0xe8400000, thumb2_disasm_ldrstrextbr },
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
    struct inst_thumb16 *t_ptr = (struct inst_thumb16 *)&tbl_thumb16;
    struct inst_arm *t2_ptr = (struct inst_arm *)&tbl_thumb32;
    UINT inst;
    WORD tinst;
    int size;
    int matched = 0;

    char tmp[64];
    DWORD_PTR* pval;

    if (!memory_get_register(CV_ARM_CPSR, &pval, tmp, sizeof(tmp)))
        dbg_printf("\n\tmemory_get_register failed: %s", tmp);
    else
        db_disasm_thumb = (*pval & 0x20) != 0;

    db_display = display;

    if (!db_disasm_thumb)
    {
        size = ARM_INSN_SIZE;
        inst = db_get_inst( memory_to_linear_addr(addr), size );
        while (a_ptr->func) {
            if ((inst & a_ptr->mask) == a_ptr->pattern) {
                    matched = 1;
                    break;
            }
            a_ptr++;
        }

        if (!matched) {
            dbg_printf("\n\tUnknown ARM Instruction: %08x", inst);
            addr->Offset += size;
        }
        else
        {
            if (!a_ptr->func(inst, addr))
                addr->Offset += size;
        }
        return;
    }
    else
    {
        WORD *taddr = memory_to_linear_addr(addr);
        tinst = db_get_inst( taddr, THUMB_INSN_SIZE );
        switch (tinst & 0xf800)
        {
            case 0xe800:
            case 0xf000:
            case 0xf800:
                size = THUMB2_INSN_SIZE;
                taddr++;
                inst = db_get_inst( taddr, THUMB_INSN_SIZE );
                inst |= (tinst << 16);

                while (t2_ptr->func) {
                    if ((inst & t2_ptr->mask) == t2_ptr->pattern) {
                            matched = 1;
                            break;
                    }
                    t2_ptr++;
                }

                if (!matched) {
                    dbg_printf("\n\tUnknown Thumb2 Instruction: %08x", inst);
                    addr->Offset += size;
                }
                else
                {
                    if (!t2_ptr->func(inst, addr))
                        addr->Offset += size;
                }
                return;
            default:
                break;
        }

        size = THUMB_INSN_SIZE;
        while (t_ptr->func) {
            if ((tinst & t_ptr->mask) == t_ptr->pattern) {
                    matched = 1;
                    break;
            }
            t_ptr++;
        }

        if (!matched) {
            dbg_printf("\n\tUnknown Thumb Instruction: %04x", tinst);
            addr->Offset += size;
        }
        else
        {
            if (!t_ptr->func(tinst, addr))
                addr->Offset += size;
        }
        return;
    }
}

static BOOL be_arm_get_addr(HANDLE hThread, const dbg_ctx_t *ctx,
                            enum be_cpu_addr bca, ADDRESS64* addr)
{
    switch (bca)
    {
    case be_cpu_addr_pc:
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->ctx.Pc);
    case be_cpu_addr_stack:
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->ctx.Sp);
    case be_cpu_addr_frame:
        return be_cpu_build_addr(hThread, ctx, addr, 0, ctx->ctx.R11);
    }
    return FALSE;
}

static BOOL be_arm_get_register_info(int regno, enum be_cpu_addr* kind)
{
    switch (regno)
    {
    case CV_ARM_PC:  *kind = be_cpu_addr_pc; return TRUE;
    case CV_ARM_R0 + 11: *kind = be_cpu_addr_frame; return TRUE;
    case CV_ARM_SP:  *kind = be_cpu_addr_stack; return TRUE;
    }
    return FALSE;
}

static void be_arm_single_step(dbg_ctx_t *ctx, BOOL enable)
{
}

static void be_arm_print_context(HANDLE hThread, const dbg_ctx_t *ctx, int all_regs)
{
    static const char condflags[] = "NZCV";
    int i;
    char        buf[8];

    switch (ctx->ctx.Cpsr & 0x1F)
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
    dbg_printf("%s %s Mode\n", (ctx->ctx.Cpsr & 0x20) ? "Thumb" : "ARM", buf);

    strcpy(buf, condflags);
    for (i = 0; buf[i]; i++)
        if (!((ctx->ctx.Cpsr >> 26) & (1 << (sizeof(condflags) - i))))
            buf[i] = '-';

    dbg_printf(" Pc:%08lx Sp:%08lx Lr:%08lx Cpsr:%08lx(%s)\n",
               ctx->ctx.Pc, ctx->ctx.Sp, ctx->ctx.Lr, ctx->ctx.Cpsr, buf);
    dbg_printf(" r0:%08lx r1:%08lx r2:%08lx r3:%08lx\n",
               ctx->ctx.R0, ctx->ctx.R1, ctx->ctx.R2, ctx->ctx.R3);
    dbg_printf(" r4:%08lx r5:%08lx r6:%08lx r7:%08lx\n",
               ctx->ctx.R4, ctx->ctx.R5, ctx->ctx.R6, ctx->ctx.R7);
    dbg_printf(" r8:%08lx r9:%08lx r10:%08lx r11:%08lx r12:%08lx\n",
               ctx->ctx.R8, ctx->ctx.R9, ctx->ctx.R10, ctx->ctx.R11, ctx->ctx.R12);

    if (all_regs) dbg_printf( "Floating point ARM dump not implemented\n" );
}

static void be_arm_print_segment_info(HANDLE hThread, const dbg_ctx_t *ctx)
{
}

static struct dbg_internal_var be_arm_ctx[] =
{
    {CV_ARM_R0 +  0,    "r0",           (void*)FIELD_OFFSET(CONTEXT, R0),     dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  1,    "r1",           (void*)FIELD_OFFSET(CONTEXT, R1),     dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  2,    "r2",           (void*)FIELD_OFFSET(CONTEXT, R2),     dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  3,    "r3",           (void*)FIELD_OFFSET(CONTEXT, R3),     dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  4,    "r4",           (void*)FIELD_OFFSET(CONTEXT, R4),     dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  5,    "r5",           (void*)FIELD_OFFSET(CONTEXT, R5),     dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  6,    "r6",           (void*)FIELD_OFFSET(CONTEXT, R6),     dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  7,    "r7",           (void*)FIELD_OFFSET(CONTEXT, R7),     dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  8,    "r8",           (void*)FIELD_OFFSET(CONTEXT, R8),     dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  9,    "r9",           (void*)FIELD_OFFSET(CONTEXT, R9),     dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  10,   "r10",          (void*)FIELD_OFFSET(CONTEXT, R10),    dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  11,   "r11",          (void*)FIELD_OFFSET(CONTEXT, R11),    dbg_itype_unsigned_int32},
    {CV_ARM_R0 +  12,   "r12",          (void*)FIELD_OFFSET(CONTEXT, R12),    dbg_itype_unsigned_int32},
    {CV_ARM_SP,         "sp",           (void*)FIELD_OFFSET(CONTEXT, Sp),     dbg_itype_unsigned_int32},
    {CV_ARM_LR,         "lr",           (void*)FIELD_OFFSET(CONTEXT, Lr),     dbg_itype_unsigned_int32},
    {CV_ARM_PC,         "pc",           (void*)FIELD_OFFSET(CONTEXT, Pc),     dbg_itype_unsigned_int32},
    {CV_ARM_CPSR,       "cpsr",         (void*)FIELD_OFFSET(CONTEXT, Cpsr),   dbg_itype_unsigned_int32},
    {0,                 NULL,           0,                                    dbg_itype_none}
};

static BOOL be_arm_is_step_over_insn(const void* insn)
{
    dbg_printf("be_arm_is_step_over_insn: not done\n");
    return FALSE;
}

static BOOL be_arm_is_function_return(const void* insn)
{
    dbg_printf("be_arm_is_function_return: not done\n");
    return FALSE;
}

static BOOL be_arm_is_break_insn(const void* insn)
{
    dbg_printf("be_arm_is_break_insn: not done\n");
    return FALSE;
}

static BOOL be_arm_is_func_call(const void* insn, ADDRESS64* callee)
{
    return FALSE;
}

static BOOL be_arm_is_jump(const void* insn, ADDRESS64* jumpee)
{
    return FALSE;
}

static BOOL be_arm_insert_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                 dbg_ctx_t *ctx, enum be_xpoint_type type,
                                 void* addr, unsigned *val, unsigned size)
{
    SIZE_T              sz;

    switch (type)
    {
    case be_xpoint_break:
        if (!size) return FALSE;
        if (!pio->read(hProcess, addr, val, 4, &sz) || sz != 4) return FALSE;
    default:
        dbg_printf("Unknown/unsupported bp type %c\n", type);
        return FALSE;
    }
    return TRUE;
}

static BOOL be_arm_remove_Xpoint(HANDLE hProcess, const struct be_process_io* pio,
                                 dbg_ctx_t *ctx, enum be_xpoint_type type,
                                 void* addr, unsigned val, unsigned size)
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

static BOOL be_arm_is_watchpoint_set(const dbg_ctx_t *ctx, unsigned idx)
{
    dbg_printf("be_arm_is_watchpoint_set: not done\n");
    return FALSE;
}

static void be_arm_clear_watchpoint(dbg_ctx_t *ctx, unsigned idx)
{
    dbg_printf("be_arm_clear_watchpoint: not done\n");
}

static int be_arm_adjust_pc_for_break(dbg_ctx_t *ctx, BOOL way)
{
    INT step = (ctx->ctx.Cpsr & 0x20) ? 2 : 4;

    if (way)
    {
        ctx->ctx.Pc -= step;
        return -step;
    }
    ctx->ctx.Pc += step;
    return step;
}

static BOOL be_arm_get_context(HANDLE thread, dbg_ctx_t *ctx)
{
    ctx->ctx.ContextFlags = CONTEXT_ALL;
    return GetThreadContext(thread, &ctx->ctx);
}

static BOOL be_arm_set_context(HANDLE thread, const dbg_ctx_t *ctx)
{
    return SetThreadContext(thread, &ctx->ctx);
}

#define REG(f,n,t,r)  {f, n, t, FIELD_OFFSET(CONTEXT, r), sizeof(((CONTEXT*)NULL)->r)}

static struct gdb_register be_arm_gdb_register_map[] = {
    REG("core", "r0",   NULL,       R0),
    REG(NULL,   "r1",   NULL,       R1),
    REG(NULL,   "r2",   NULL,       R2),
    REG(NULL,   "r3",   NULL,       R3),
    REG(NULL,   "r4",   NULL,       R4),
    REG(NULL,   "r5",   NULL,       R5),
    REG(NULL,   "r6",   NULL,       R6),
    REG(NULL,   "r7",   NULL,       R7),
    REG(NULL,   "r8",   NULL,       R8),
    REG(NULL,   "r9",   NULL,       R9),
    REG(NULL,   "r10",  NULL,       R10),
    REG(NULL,   "r11",  NULL,       R11),
    REG(NULL,   "r12",  NULL,       R12),
    REG(NULL,   "sp",   "data_ptr", Sp),
    REG(NULL,   "lr",   "code_ptr", Lr),
    REG(NULL,   "pc",   "code_ptr", Pc),
    REG(NULL,   "cpsr", NULL,       Cpsr),
};

struct backend_cpu be_arm =
{
    IMAGE_FILE_MACHINE_ARMNT,
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
    be_arm_get_context,
    be_arm_set_context,
    be_arm_gdb_register_map,
    ARRAY_SIZE(be_arm_gdb_register_map),
};
#endif
