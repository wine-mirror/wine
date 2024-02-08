/*
 * DWARF unwinding routines
 *
 * Copyright 2009 Alexandre Julliard
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

#ifndef __NTDLL_DWARF_H
#define __NTDLL_DWARF_H

#define WIN32_NO_STATUS
#include "windef.h"
#include "winnt.h"

/***********************************************************************
 * Definitions for Dwarf unwind tables
 */

#define DW_CFA_advance_loc 0x40
#define DW_CFA_offset 0x80
#define DW_CFA_restore 0xc0
#define DW_CFA_nop 0x00
#define DW_CFA_set_loc 0x01
#define DW_CFA_advance_loc1 0x02
#define DW_CFA_advance_loc2 0x03
#define DW_CFA_advance_loc4 0x04
#define DW_CFA_offset_extended 0x05
#define DW_CFA_restore_extended 0x06
#define DW_CFA_undefined 0x07
#define DW_CFA_same_value 0x08
#define DW_CFA_register 0x09
#define DW_CFA_remember_state 0x0a
#define DW_CFA_restore_state 0x0b
#define DW_CFA_def_cfa 0x0c
#define DW_CFA_def_cfa_register 0x0d
#define DW_CFA_def_cfa_offset 0x0e
#define DW_CFA_def_cfa_expression 0x0f
#define DW_CFA_expression 0x10
#define DW_CFA_offset_extended_sf 0x11
#define DW_CFA_def_cfa_sf 0x12
#define DW_CFA_def_cfa_offset_sf 0x13
#define DW_CFA_val_offset 0x14
#define DW_CFA_val_offset_sf 0x15
#define DW_CFA_val_expression 0x16

#define DW_OP_addr                 0x03
#define DW_OP_deref                0x06
#define DW_OP_const1u              0x08
#define DW_OP_const1s              0x09
#define DW_OP_const2u              0x0a
#define DW_OP_const2s              0x0b
#define DW_OP_const4u              0x0c
#define DW_OP_const4s              0x0d
#define DW_OP_const8u              0x0e
#define DW_OP_const8s              0x0f
#define DW_OP_constu               0x10
#define DW_OP_consts               0x11
#define DW_OP_dup                  0x12
#define DW_OP_drop                 0x13
#define DW_OP_over                 0x14
#define DW_OP_pick                 0x15
#define DW_OP_swap                 0x16
#define DW_OP_rot                  0x17
#define DW_OP_xderef               0x18
#define DW_OP_abs                  0x19
#define DW_OP_and                  0x1a
#define DW_OP_div                  0x1b
#define DW_OP_minus                0x1c
#define DW_OP_mod                  0x1d
#define DW_OP_mul                  0x1e
#define DW_OP_neg                  0x1f
#define DW_OP_not                  0x20
#define DW_OP_or                   0x21
#define DW_OP_plus                 0x22
#define DW_OP_plus_uconst          0x23
#define DW_OP_shl                  0x24
#define DW_OP_shr                  0x25
#define DW_OP_shra                 0x26
#define DW_OP_xor                  0x27
#define DW_OP_bra                  0x28
#define DW_OP_eq                   0x29
#define DW_OP_ge                   0x2a
#define DW_OP_gt                   0x2b
#define DW_OP_le                   0x2c
#define DW_OP_lt                   0x2d
#define DW_OP_ne                   0x2e
#define DW_OP_skip                 0x2f
#define DW_OP_lit0                 0x30
#define DW_OP_lit1                 0x31
#define DW_OP_lit2                 0x32
#define DW_OP_lit3                 0x33
#define DW_OP_lit4                 0x34
#define DW_OP_lit5                 0x35
#define DW_OP_lit6                 0x36
#define DW_OP_lit7                 0x37
#define DW_OP_lit8                 0x38
#define DW_OP_lit9                 0x39
#define DW_OP_lit10                0x3a
#define DW_OP_lit11                0x3b
#define DW_OP_lit12                0x3c
#define DW_OP_lit13                0x3d
#define DW_OP_lit14                0x3e
#define DW_OP_lit15                0x3f
#define DW_OP_lit16                0x40
#define DW_OP_lit17                0x41
#define DW_OP_lit18                0x42
#define DW_OP_lit19                0x43
#define DW_OP_lit20                0x44
#define DW_OP_lit21                0x45
#define DW_OP_lit22                0x46
#define DW_OP_lit23                0x47
#define DW_OP_lit24                0x48
#define DW_OP_lit25                0x49
#define DW_OP_lit26                0x4a
#define DW_OP_lit27                0x4b
#define DW_OP_lit28                0x4c
#define DW_OP_lit29                0x4d
#define DW_OP_lit30                0x4e
#define DW_OP_lit31                0x4f
#define DW_OP_reg0                 0x50
#define DW_OP_reg1                 0x51
#define DW_OP_reg2                 0x52
#define DW_OP_reg3                 0x53
#define DW_OP_reg4                 0x54
#define DW_OP_reg5                 0x55
#define DW_OP_reg6                 0x56
#define DW_OP_reg7                 0x57
#define DW_OP_reg8                 0x58
#define DW_OP_reg9                 0x59
#define DW_OP_reg10                0x5a
#define DW_OP_reg11                0x5b
#define DW_OP_reg12                0x5c
#define DW_OP_reg13                0x5d
#define DW_OP_reg14                0x5e
#define DW_OP_reg15                0x5f
#define DW_OP_reg16                0x60
#define DW_OP_reg17                0x61
#define DW_OP_reg18                0x62
#define DW_OP_reg19                0x63
#define DW_OP_reg20                0x64
#define DW_OP_reg21                0x65
#define DW_OP_reg22                0x66
#define DW_OP_reg23                0x67
#define DW_OP_reg24                0x68
#define DW_OP_reg25                0x69
#define DW_OP_reg26                0x6a
#define DW_OP_reg27                0x6b
#define DW_OP_reg28                0x6c
#define DW_OP_reg29                0x6d
#define DW_OP_reg30                0x6e
#define DW_OP_reg31                0x6f
#define DW_OP_breg0                0x70
#define DW_OP_breg1                0x71
#define DW_OP_breg2                0x72
#define DW_OP_breg3                0x73
#define DW_OP_breg4                0x74
#define DW_OP_breg5                0x75
#define DW_OP_breg6                0x76
#define DW_OP_breg7                0x77
#define DW_OP_breg8                0x78
#define DW_OP_breg9                0x79
#define DW_OP_breg10               0x7a
#define DW_OP_breg11               0x7b
#define DW_OP_breg12               0x7c
#define DW_OP_breg13               0x7d
#define DW_OP_breg14               0x7e
#define DW_OP_breg15               0x7f
#define DW_OP_breg16               0x80
#define DW_OP_breg17               0x81
#define DW_OP_breg18               0x82
#define DW_OP_breg19               0x83
#define DW_OP_breg20               0x84
#define DW_OP_breg21               0x85
#define DW_OP_breg22               0x86
#define DW_OP_breg23               0x87
#define DW_OP_breg24               0x88
#define DW_OP_breg25               0x89
#define DW_OP_breg26               0x8a
#define DW_OP_breg27               0x8b
#define DW_OP_breg28               0x8c
#define DW_OP_breg29               0x8d
#define DW_OP_breg30               0x8e
#define DW_OP_breg31               0x8f
#define DW_OP_regx                 0x90
#define DW_OP_fbreg                0x91
#define DW_OP_bregx                0x92
#define DW_OP_piece                0x93
#define DW_OP_deref_size           0x94
#define DW_OP_xderef_size          0x95
#define DW_OP_nop                  0x96
#define DW_OP_push_object_address  0x97
#define DW_OP_call2                0x98
#define DW_OP_call4                0x99
#define DW_OP_call_ref             0x9a
#define DW_OP_form_tls_address     0x9b
#define DW_OP_call_frame_cfa       0x9c
#define DW_OP_bit_piece            0x9d
#define DW_OP_lo_user              0xe0
#define DW_OP_hi_user              0xff
#define DW_OP_GNU_push_tls_address 0xe0
#define DW_OP_GNU_uninit           0xf0
#define DW_OP_GNU_encoded_addr     0xf1

#ifndef NTDLL_DWARF_H_NO_UNWINDER

#define DW_EH_PE_native   0x00
#define DW_EH_PE_uleb128  0x01
#define DW_EH_PE_udata2   0x02
#define DW_EH_PE_udata4   0x03
#define DW_EH_PE_udata8   0x04
#define DW_EH_PE_sleb128  0x09
#define DW_EH_PE_sdata2   0x0a
#define DW_EH_PE_sdata4   0x0b
#define DW_EH_PE_sdata8   0x0c
#define DW_EH_PE_signed   0x08
#define DW_EH_PE_abs      0x00
#define DW_EH_PE_pcrel    0x10
#define DW_EH_PE_textrel  0x20
#define DW_EH_PE_datarel  0x30
#define DW_EH_PE_funcrel  0x40
#define DW_EH_PE_aligned  0x50
#define DW_EH_PE_indirect 0x80
#define DW_EH_PE_omit     0xff

struct dwarf_eh_bases
{
    void *tbase;
    void *dbase;
    void *func;
};

struct dwarf_cie
{
    unsigned int  length;
    int           id;
    unsigned char version;
    unsigned char augmentation[1];
};

struct dwarf_fde
{
    unsigned int length;
    unsigned int cie_offset;
};

extern const struct dwarf_fde *_Unwind_Find_FDE (void *, struct dwarf_eh_bases *);

static unsigned char dwarf_get_u1( const unsigned char **p )
{
    return *(*p)++;
}

static unsigned short dwarf_get_u2( const unsigned char **p )
{
    unsigned int ret = (*p)[0] | ((*p)[1] << 8);
    (*p) += 2;
    return ret;
}

static unsigned int dwarf_get_u4( const unsigned char **p )
{
    unsigned int ret = (*p)[0] | ((*p)[1] << 8) | ((*p)[2] << 16) | ((*p)[3] << 24);
    (*p) += 4;
    return ret;
}

static ULONG64 dwarf_get_u8( const unsigned char **p )
{
    ULONG64 low  = dwarf_get_u4( p );
    ULONG64 high = dwarf_get_u4( p );
    return low | (high << 32);
}

static ULONG_PTR dwarf_get_uleb128( const unsigned char **p )
{
    ULONG_PTR ret = 0;
    unsigned int shift = 0;
    unsigned char byte;

    do
    {
        byte = **p;
        ret |= (ULONG_PTR)(byte & 0x7f) << shift;
        shift += 7;
        (*p)++;
    } while (byte & 0x80);
    return ret;
}

static LONG_PTR dwarf_get_sleb128( const unsigned char **p )
{
    ULONG_PTR ret = 0;
    unsigned int shift = 0;
    unsigned char byte;

    do
    {
        byte = **p;
        ret |= (ULONG_PTR)(byte & 0x7f) << shift;
        shift += 7;
        (*p)++;
    } while (byte & 0x80);

    if ((shift < 8 * sizeof(ret)) && (byte & 0x40)) ret |= -((ULONG_PTR)1 << shift);
    return ret;
}

static ULONG_PTR dwarf_get_ptr( const unsigned char **p, unsigned char encoding, const struct dwarf_eh_bases *bases )
{
    ULONG_PTR base;

    if (encoding == DW_EH_PE_omit) return 0;

    switch (encoding & 0x70)
    {
    case DW_EH_PE_abs:
        base = 0;
        break;
    case DW_EH_PE_pcrel:
        base = (ULONG_PTR)*p;
        break;
    case DW_EH_PE_textrel:
        base = (ULONG_PTR)bases->tbase;
        break;
    case DW_EH_PE_datarel:
        base = (ULONG_PTR)bases->dbase;
        break;
    case DW_EH_PE_funcrel:
        base = (ULONG_PTR)bases->func;
        break;
    case DW_EH_PE_aligned:
        base = ((ULONG_PTR)*p + 7) & ~7ul;
        break;
    default:
        FIXME( "unsupported encoding %02x\n", encoding );
        return 0;
    }

    switch (encoding & 0x0f)
    {
    case DW_EH_PE_native:  base += dwarf_get_u8( p ); break;
    case DW_EH_PE_uleb128: base += dwarf_get_uleb128( p ); break;
    case DW_EH_PE_udata2:  base += dwarf_get_u2( p ); break;
    case DW_EH_PE_udata4:  base += dwarf_get_u4( p ); break;
    case DW_EH_PE_udata8:  base += dwarf_get_u8( p ); break;
    case DW_EH_PE_sleb128: base += dwarf_get_sleb128( p ); break;
    case DW_EH_PE_sdata2:  base += (signed short)dwarf_get_u2( p ); break;
    case DW_EH_PE_sdata4:  base += (signed int)dwarf_get_u4( p ); break;
    case DW_EH_PE_sdata8:  base += (LONG64)dwarf_get_u8( p ); break;
    default:
        FIXME( "unsupported encoding %02x\n", encoding );
        return 0;
    }
    if (encoding & DW_EH_PE_indirect) base = *(ULONG_PTR *)base;
    return base;
}

enum reg_rule
{
    RULE_UNSET,          /* not set at all */
    RULE_UNDEFINED,      /* undefined value */
    RULE_SAME,           /* same value as previous frame */
    RULE_CFA_OFFSET,     /* stored at cfa offset */
    RULE_OTHER_REG,      /* stored in other register */
    RULE_EXPRESSION,     /* address specified by expression */
    RULE_VAL_EXPRESSION  /* value specified by expression */
};

#ifdef __x86_64__
#define NB_FRAME_REGS 41
#elif defined(__aarch64__)
#define NB_FRAME_REGS 96
#else
#error Unsupported architecture
#endif
#define MAX_SAVED_STATES 16

struct frame_state
{
    ULONG_PTR     cfa_offset;
    unsigned char cfa_reg;
    enum reg_rule cfa_rule;
    enum reg_rule rules[NB_FRAME_REGS];
    ULONG64       regs[NB_FRAME_REGS];
};

struct frame_info
{
    ULONG_PTR     ip;
    ULONG_PTR     code_align;
    LONG_PTR      data_align;
    unsigned char retaddr_reg;
    unsigned char fde_encoding;
    unsigned char signal_frame;
    unsigned char state_sp;
    struct frame_state state;
    struct frame_state *state_stack;
};

static const char *dwarf_reg_names[NB_FRAME_REGS] =
{
#ifdef __x86_64__
/*  0-7  */ "%rax", "%rdx", "%rcx", "%rbx", "%rsi", "%rdi", "%rbp", "%rsp",
/*  8-16 */ "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15", "%rip",
/* 17-24 */ "%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7",
/* 25-32 */ "%xmm8", "%xmm9", "%xmm10", "%xmm11", "%xmm12", "%xmm13", "%xmm14", "%xmm15",
/* 33-40 */ "%st0", "%st1", "%st2", "%st3", "%st4", "%st5", "%st6", "%st7"
#elif defined(__aarch64__)
/*  0-7  */ "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",
/*  8-15 */ "x8",  "x9",  "x10", "x11", "x12", "x13", "x14", "x15",
/* 16-23 */ "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
/* 24-31 */ "x24", "x25", "x26", "x27", "x28", "x29", "x30", "sp",
/* 32-39 */ "pc",  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,
/* 40-47 */ NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,
/* 48-55 */ NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,
/* 56-63 */ NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,
/* 64-71 */ "v0",  "v1",  "v2",  "v3",  "v4",  "v5",  "v6",  "v7",
/* 72-79 */ "v8",  "v9",  "v10", "v11", "v12", "v13", "v14", "v15",
/* 80-87 */ "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23",
/* 88-95 */ "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31",
#endif
};

static BOOL valid_reg( ULONG_PTR reg )
{
    if (reg >= NB_FRAME_REGS) FIXME( "unsupported reg %lx\n", reg );
    return (reg < NB_FRAME_REGS);
}

static void execute_cfa_instructions( const unsigned char *ptr, const unsigned char *end,
                                      ULONG_PTR last_ip, struct frame_info *info,
                                      const struct dwarf_eh_bases *bases )
{
    while (ptr < end && info->ip < last_ip + info->signal_frame)
    {
        const unsigned char op = *ptr++;

        if (op & 0xc0)
        {
            switch (op & 0xc0)
            {
            case DW_CFA_advance_loc:
            {
                ULONG_PTR offset = (op & 0x3f) * info->code_align;
                TRACE( "%lx: DW_CFA_advance_loc %lu\n", info->ip, offset );
                info->ip += offset;
                break;
            }
            case DW_CFA_offset:
            {
                ULONG_PTR reg = op & 0x3f;
                LONG_PTR offset = dwarf_get_uleb128( &ptr ) * info->data_align;
                if (!valid_reg( reg )) break;
                TRACE( "%lx: DW_CFA_offset %s, %ld\n", info->ip, dwarf_reg_names[reg], offset );
                info->state.regs[reg]  = offset;
                info->state.rules[reg] = RULE_CFA_OFFSET;
                break;
            }
            case DW_CFA_restore:
            {
                ULONG_PTR reg = op & 0x3f;
                if (!valid_reg( reg )) break;
                TRACE( "%lx: DW_CFA_restore %s\n", info->ip, dwarf_reg_names[reg] );
                info->state.rules[reg] = RULE_UNSET;
                break;
            }
            }
        }
        else switch (op)
        {
        case DW_CFA_nop:
            break;
        case DW_CFA_set_loc:
        {
            ULONG_PTR loc = dwarf_get_ptr( &ptr, info->fde_encoding, bases );
            TRACE( "%lx: DW_CFA_set_loc %lx\n", info->ip, loc );
            info->ip = loc;
            break;
        }
        case DW_CFA_advance_loc1:
        {
            ULONG_PTR offset = *ptr++ * info->code_align;
            TRACE( "%lx: DW_CFA_advance_loc1 %lu\n", info->ip, offset );
            info->ip += offset;
            break;
        }
        case DW_CFA_advance_loc2:
        {
            ULONG_PTR offset = dwarf_get_u2( &ptr ) * info->code_align;
            TRACE( "%lx: DW_CFA_advance_loc2 %lu\n", info->ip, offset );
            info->ip += offset;
            break;
        }
        case DW_CFA_advance_loc4:
        {
            ULONG_PTR offset = dwarf_get_u4( &ptr ) * info->code_align;
            TRACE( "%lx: DW_CFA_advance_loc4 %lu\n", info->ip, offset );
            info->ip += offset;
            break;
        }
        case DW_CFA_offset_extended:
        case DW_CFA_offset_extended_sf:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            LONG_PTR offset = (op == DW_CFA_offset_extended) ? dwarf_get_uleb128( &ptr ) * info->data_align
                                                             : dwarf_get_sleb128( &ptr ) * info->data_align;
            if (!valid_reg( reg )) break;
            TRACE( "%lx: DW_CFA_offset_extended %s, %ld\n", info->ip, dwarf_reg_names[reg], offset );
            info->state.regs[reg]  = offset;
            info->state.rules[reg] = RULE_CFA_OFFSET;
            break;
        }
        case DW_CFA_restore_extended:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            if (!valid_reg( reg )) break;
            TRACE( "%lx: DW_CFA_restore_extended %s\n", info->ip, dwarf_reg_names[reg] );
            info->state.rules[reg] = RULE_UNSET;
            break;
        }
        case DW_CFA_undefined:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            if (!valid_reg( reg )) break;
            TRACE( "%lx: DW_CFA_undefined %s\n", info->ip, dwarf_reg_names[reg] );
            info->state.rules[reg] = RULE_UNDEFINED;
            break;
        }
        case DW_CFA_same_value:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            if (!valid_reg( reg )) break;
            TRACE( "%lx: DW_CFA_same_value %s\n", info->ip, dwarf_reg_names[reg] );
            info->state.regs[reg]  = reg;
            info->state.rules[reg] = RULE_SAME;
            break;
        }
        case DW_CFA_register:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            ULONG_PTR reg2 = dwarf_get_uleb128( &ptr );
            if (!valid_reg( reg ) || !valid_reg( reg2 )) break;
            TRACE( "%lx: DW_CFA_register %s == %s\n", info->ip, dwarf_reg_names[reg], dwarf_reg_names[reg2] );
            info->state.regs[reg]  = reg2;
            info->state.rules[reg] = RULE_OTHER_REG;
            break;
        }
        case DW_CFA_remember_state:
            TRACE( "%lx: DW_CFA_remember_state\n", info->ip );
            if (info->state_sp >= MAX_SAVED_STATES)
                FIXME( "%lx: DW_CFA_remember_state too many nested saves\n", info->ip );
            else
                info->state_stack[info->state_sp++] = info->state;
            break;
        case DW_CFA_restore_state:
            TRACE( "%lx: DW_CFA_restore_state\n", info->ip );
            if (!info->state_sp)
                FIXME( "%lx: DW_CFA_restore_state without corresponding save\n", info->ip );
            else
                info->state = info->state_stack[--info->state_sp];
            break;
        case DW_CFA_def_cfa:
        case DW_CFA_def_cfa_sf:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            ULONG_PTR offset = (op == DW_CFA_def_cfa) ? dwarf_get_uleb128( &ptr )
                                                      : dwarf_get_sleb128( &ptr ) * info->data_align;
            if (!valid_reg( reg )) break;
            TRACE( "%lx: DW_CFA_def_cfa %s, %lu\n", info->ip, dwarf_reg_names[reg], offset );
            info->state.cfa_reg    = reg;
            info->state.cfa_offset = offset;
            info->state.cfa_rule   = RULE_CFA_OFFSET;
            break;
        }
        case DW_CFA_def_cfa_register:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            if (!valid_reg( reg )) break;
            TRACE( "%lx: DW_CFA_def_cfa_register %s\n", info->ip, dwarf_reg_names[reg] );
            info->state.cfa_reg = reg;
            info->state.cfa_rule = RULE_CFA_OFFSET;
            break;
        }
        case DW_CFA_def_cfa_offset:
        case DW_CFA_def_cfa_offset_sf:
        {
            ULONG_PTR offset = (op == DW_CFA_def_cfa_offset) ? dwarf_get_uleb128( &ptr )
                                                             : dwarf_get_sleb128( &ptr ) * info->data_align;
            TRACE( "%lx: DW_CFA_def_cfa_offset %lu\n", info->ip, offset );
            info->state.cfa_offset = offset;
            info->state.cfa_rule = RULE_CFA_OFFSET;
            break;
        }
        case DW_CFA_def_cfa_expression:
        {
            ULONG_PTR expr = (ULONG_PTR)ptr;
            ULONG_PTR len = dwarf_get_uleb128( &ptr );
            TRACE( "%lx: DW_CFA_def_cfa_expression %lx-%lx\n", info->ip, expr, expr+len );
            info->state.cfa_offset = expr;
            info->state.cfa_rule = RULE_VAL_EXPRESSION;
            ptr += len;
            break;
        }
        case DW_CFA_expression:
        case DW_CFA_val_expression:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            ULONG_PTR expr = (ULONG_PTR)ptr;
            ULONG_PTR len = dwarf_get_uleb128( &ptr );
            if (!valid_reg( reg )) break;
            TRACE( "%lx: DW_CFA_%sexpression %s %lx-%lx\n",
                   info->ip, (op == DW_CFA_expression) ? "" : "val_", dwarf_reg_names[reg], expr, expr+len );
            info->state.regs[reg]  = expr;
            info->state.rules[reg] = (op == DW_CFA_expression) ? RULE_EXPRESSION : RULE_VAL_EXPRESSION;
            ptr += len;
            break;
        }
        default:
            FIXME( "%lx: unknown CFA opcode %02x\n", info->ip, op );
            break;
        }
    }
}

/* retrieve a context register from its dwarf number */
static void *get_context_reg( CONTEXT *context, ULONG_PTR dw_reg )
{
    switch (dw_reg)
    {
#ifdef __x86_64__
    case 0:  return &context->Rax;
    case 1:  return &context->Rdx;
    case 2:  return &context->Rcx;
    case 3:  return &context->Rbx;
    case 4:  return &context->Rsi;
    case 5:  return &context->Rdi;
    case 6:  return &context->Rbp;
    case 7:  return &context->Rsp;
    case 8:  return &context->R8;
    case 9:  return &context->R9;
    case 10: return &context->R10;
    case 11: return &context->R11;
    case 12: return &context->R12;
    case 13: return &context->R13;
    case 14: return &context->R14;
    case 15: return &context->R15;
    case 16: return &context->Rip;
    case 17: return &context->Xmm0;
    case 18: return &context->Xmm1;
    case 19: return &context->Xmm2;
    case 20: return &context->Xmm3;
    case 21: return &context->Xmm4;
    case 22: return &context->Xmm5;
    case 23: return &context->Xmm6;
    case 24: return &context->Xmm7;
    case 25: return &context->Xmm8;
    case 26: return &context->Xmm9;
    case 27: return &context->Xmm10;
    case 28: return &context->Xmm11;
    case 29: return &context->Xmm12;
    case 30: return &context->Xmm13;
    case 31: return &context->Xmm14;
    case 32: return &context->Xmm15;
    case 33: return &context->Legacy[0];
    case 34: return &context->Legacy[1];
    case 35: return &context->Legacy[2];
    case 36: return &context->Legacy[3];
    case 37: return &context->Legacy[4];
    case 38: return &context->Legacy[5];
    case 39: return &context->Legacy[6];
    case 40: return &context->Legacy[7];
#elif defined(__aarch64__)
    case 0:  return &context->X0;
    case 1:  return &context->X1;
    case 2:  return &context->X2;
    case 3:  return &context->X3;
    case 4:  return &context->X4;
    case 5:  return &context->X5;
    case 6:  return &context->X6;
    case 7:  return &context->X7;
    case 8:  return &context->X8;
    case 9:  return &context->X9;
    case 10: return &context->X10;
    case 11: return &context->X11;
    case 12: return &context->X12;
    case 13: return &context->X13;
    case 14: return &context->X14;
    case 15: return &context->X15;
    case 16: return &context->X16;
    case 17: return &context->X17;
    case 18: return &context->X18;
    case 19: return &context->X19;
    case 20: return &context->X20;
    case 21: return &context->X21;
    case 22: return &context->X22;
    case 23: return &context->X23;
    case 24: return &context->X24;
    case 25: return &context->X25;
    case 26: return &context->X26;
    case 27: return &context->X27;
    case 28: return &context->X28;
    case 29: return &context->Fp;
    case 30: return &context->Lr;
    case 31: return &context->Sp;
    case 32: return &context->Pc;
    case 64:
    case 65:
    case 66:
    case 67:
    case 68:
    case 69:
    case 70:
    case 71:
    case 72:
    case 73:
    case 74:
    case 75:
    case 76:
    case 77:
    case 78:
    case 79:
    case 80:
    case 81:
    case 82:
    case 83:
    case 84:
    case 85:
    case 86:
    case 87:
    case 88:
    case 89:
    case 90:
    case 91:
    case 92:
    case 93:
    case 94:
    case 95:
        return &context->V[dw_reg - 64];
#endif
    default: return NULL;
    }
}

/* set a context register from its dwarf number */
static void set_context_reg( CONTEXT *context, ULONG_PTR dw_reg, void *val )
{
    switch (dw_reg)
    {
#ifdef __x86_64__
    case 0:  context->Rax = *(ULONG64 *)val; break;
    case 1:  context->Rdx = *(ULONG64 *)val; break;
    case 2:  context->Rcx = *(ULONG64 *)val; break;
    case 3:  context->Rbx = *(ULONG64 *)val; break;
    case 4:  context->Rsi = *(ULONG64 *)val; break;
    case 5:  context->Rdi = *(ULONG64 *)val; break;
    case 6:  context->Rbp = *(ULONG64 *)val; break;
    case 7:  context->Rsp = *(ULONG64 *)val; break;
    case 8:  context->R8  = *(ULONG64 *)val; break;
    case 9:  context->R9  = *(ULONG64 *)val; break;
    case 10: context->R10 = *(ULONG64 *)val; break;
    case 11: context->R11 = *(ULONG64 *)val; break;
    case 12: context->R12 = *(ULONG64 *)val; break;
    case 13: context->R13 = *(ULONG64 *)val; break;
    case 14: context->R14 = *(ULONG64 *)val; break;
    case 15: context->R15 = *(ULONG64 *)val; break;
    case 16: context->Rip = *(ULONG64 *)val; break;
    case 17: memcpy( &context->Xmm0, val, sizeof(M128A) ); break;
    case 18: memcpy( &context->Xmm1, val, sizeof(M128A) ); break;
    case 19: memcpy( &context->Xmm2, val, sizeof(M128A) ); break;
    case 20: memcpy( &context->Xmm3, val, sizeof(M128A) ); break;
    case 21: memcpy( &context->Xmm4, val, sizeof(M128A) ); break;
    case 22: memcpy( &context->Xmm5, val, sizeof(M128A) ); break;
    case 23: memcpy( &context->Xmm6, val, sizeof(M128A) ); break;
    case 24: memcpy( &context->Xmm7, val, sizeof(M128A) ); break;
    case 25: memcpy( &context->Xmm8, val, sizeof(M128A) ); break;
    case 26: memcpy( &context->Xmm9, val, sizeof(M128A) ); break;
    case 27: memcpy( &context->Xmm10, val, sizeof(M128A) ); break;
    case 28: memcpy( &context->Xmm11, val, sizeof(M128A) ); break;
    case 29: memcpy( &context->Xmm12, val, sizeof(M128A) ); break;
    case 30: memcpy( &context->Xmm13, val, sizeof(M128A) ); break;
    case 31: memcpy( &context->Xmm14, val, sizeof(M128A) ); break;
    case 32: memcpy( &context->Xmm15, val, sizeof(M128A) ); break;
    case 33: memcpy( &context->Legacy[0], val, sizeof(M128A) ); break;
    case 34: memcpy( &context->Legacy[1], val, sizeof(M128A) ); break;
    case 35: memcpy( &context->Legacy[2], val, sizeof(M128A) ); break;
    case 36: memcpy( &context->Legacy[3], val, sizeof(M128A) ); break;
    case 37: memcpy( &context->Legacy[4], val, sizeof(M128A) ); break;
    case 38: memcpy( &context->Legacy[5], val, sizeof(M128A) ); break;
    case 39: memcpy( &context->Legacy[6], val, sizeof(M128A) ); break;
    case 40: memcpy( &context->Legacy[7], val, sizeof(M128A) ); break;
#elif defined(__aarch64__)
    case 0:  context->X0  = *(DWORD64 *)val; break;
    case 1:  context->X1  = *(DWORD64 *)val; break;
    case 2:  context->X2  = *(DWORD64 *)val; break;
    case 3:  context->X3  = *(DWORD64 *)val; break;
    case 4:  context->X4  = *(DWORD64 *)val; break;
    case 5:  context->X5  = *(DWORD64 *)val; break;
    case 6:  context->X6  = *(DWORD64 *)val; break;
    case 7:  context->X7  = *(DWORD64 *)val; break;
    case 8:  context->X8  = *(DWORD64 *)val; break;
    case 9:  context->X9  = *(DWORD64 *)val; break;
    case 10: context->X10 = *(DWORD64 *)val; break;
    case 11: context->X11 = *(DWORD64 *)val; break;
    case 12: context->X12 = *(DWORD64 *)val; break;
    case 13: context->X13 = *(DWORD64 *)val; break;
    case 14: context->X14 = *(DWORD64 *)val; break;
    case 15: context->X15 = *(DWORD64 *)val; break;
    case 16: context->X16 = *(DWORD64 *)val; break;
    case 17: context->X17 = *(DWORD64 *)val; break;
    case 18: context->X18 = *(DWORD64 *)val; break;
    case 19: context->X19 = *(DWORD64 *)val; break;
    case 20: context->X20 = *(DWORD64 *)val; break;
    case 21: context->X21 = *(DWORD64 *)val; break;
    case 22: context->X22 = *(DWORD64 *)val; break;
    case 23: context->X23 = *(DWORD64 *)val; break;
    case 24: context->X24 = *(DWORD64 *)val; break;
    case 25: context->X25 = *(DWORD64 *)val; break;
    case 26: context->X26 = *(DWORD64 *)val; break;
    case 27: context->X27 = *(DWORD64 *)val; break;
    case 28: context->X28 = *(DWORD64 *)val; break;
    case 29: context->Fp  = *(DWORD64 *)val; break;
    case 30: context->Lr  = *(DWORD64 *)val; break;
    case 31: context->Sp  = *(DWORD64 *)val; break;
    case 32: context->Pc  = *(DWORD64 *)val; break;
    case 64:
    case 65:
    case 66:
    case 67:
    case 68:
    case 69:
    case 70:
    case 71:
    case 72:
    case 73:
    case 74:
    case 75:
    case 76:
    case 77:
    case 78:
    case 79:
    case 80:
    case 81:
    case 82:
    case 83:
    case 84:
    case 85:
    case 86:
    case 87:
    case 88:
    case 89:
    case 90:
    case 91:
    case 92:
    case 93:
    case 94:
    case 95:
        memcpy( &context->V[dw_reg - 64], val, sizeof(ARM64_NT_NEON128) );
        break;
#endif
    }
}

static ULONG_PTR eval_expression( const unsigned char *p, CONTEXT *context,
                                  const struct dwarf_eh_bases *bases )
{
    ULONG_PTR reg, tmp, stack[64];
    int sp = -1;
    ULONG_PTR len = dwarf_get_uleb128(&p);
    const unsigned char *end = p + len;

    while (p < end)
    {
        unsigned char opcode = dwarf_get_u1(&p);

        if (opcode >= DW_OP_lit0 && opcode <= DW_OP_lit31)
            stack[++sp] = opcode - DW_OP_lit0;
        else if (opcode >= DW_OP_reg0 && opcode <= DW_OP_reg31)
            stack[++sp] = *(ULONG_PTR *)get_context_reg( context, opcode - DW_OP_reg0 );
        else if (opcode >= DW_OP_breg0 && opcode <= DW_OP_breg31)
            stack[++sp] = *(ULONG_PTR *)get_context_reg( context, opcode - DW_OP_breg0 ) + dwarf_get_sleb128(&p);
        else switch (opcode)
        {
        case DW_OP_nop:         break;
        case DW_OP_addr:        stack[++sp] = dwarf_get_u8(&p); break;
        case DW_OP_const1u:     stack[++sp] = dwarf_get_u1(&p); break;
        case DW_OP_const1s:     stack[++sp] = (signed char)dwarf_get_u1(&p); break;
        case DW_OP_const2u:     stack[++sp] = dwarf_get_u2(&p); break;
        case DW_OP_const2s:     stack[++sp] = (short)dwarf_get_u2(&p); break;
        case DW_OP_const4u:     stack[++sp] = dwarf_get_u4(&p); break;
        case DW_OP_const4s:     stack[++sp] = (signed int)dwarf_get_u4(&p); break;
        case DW_OP_const8u:     stack[++sp] = dwarf_get_u8(&p); break;
        case DW_OP_const8s:     stack[++sp] = (LONG_PTR)dwarf_get_u8(&p); break;
        case DW_OP_constu:      stack[++sp] = dwarf_get_uleb128(&p); break;
        case DW_OP_consts:      stack[++sp] = dwarf_get_sleb128(&p); break;
        case DW_OP_deref:       stack[sp] = *(ULONG_PTR *)stack[sp]; break;
        case DW_OP_dup:         stack[sp + 1] = stack[sp]; sp++; break;
        case DW_OP_drop:        sp--; break;
        case DW_OP_over:        stack[sp + 1] = stack[sp - 1]; sp++; break;
        case DW_OP_pick:        stack[sp + 1] = stack[sp - dwarf_get_u1(&p)]; sp++; break;
        case DW_OP_swap:        tmp = stack[sp]; stack[sp] = stack[sp-1]; stack[sp-1] = tmp; break;
        case DW_OP_rot:         tmp = stack[sp]; stack[sp] = stack[sp-1]; stack[sp-1] = stack[sp-2]; stack[sp-2] = tmp; break;
        case DW_OP_abs:         stack[sp] = labs((LONG_PTR)stack[sp]); break;
        case DW_OP_neg:         stack[sp] = -stack[sp]; break;
        case DW_OP_not:         stack[sp] = ~stack[sp]; break;
        case DW_OP_and:         stack[sp-1] &= stack[sp]; sp--; break;
        case DW_OP_or:          stack[sp-1] |= stack[sp]; sp--; break;
        case DW_OP_minus:       stack[sp-1] -= stack[sp]; sp--; break;
        case DW_OP_mul:         stack[sp-1] *= stack[sp]; sp--; break;
        case DW_OP_plus:        stack[sp-1] += stack[sp]; sp--; break;
        case DW_OP_xor:         stack[sp-1] ^= stack[sp]; sp--; break;
        case DW_OP_shl:         stack[sp-1] <<= stack[sp]; sp--; break;
        case DW_OP_shr:         stack[sp-1] >>= stack[sp]; sp--; break;
        case DW_OP_plus_uconst: stack[sp] += dwarf_get_uleb128(&p); break;
        case DW_OP_shra:        stack[sp-1] = (LONG_PTR)stack[sp-1] / (1 << stack[sp]); sp--; break;
        case DW_OP_div:         stack[sp-1] = (LONG_PTR)stack[sp-1] / (LONG_PTR)stack[sp]; sp--; break;
        case DW_OP_mod:         stack[sp-1] = (LONG_PTR)stack[sp-1] % (LONG_PTR)stack[sp]; sp--; break;
        case DW_OP_ge:          stack[sp-1] = ((LONG_PTR)stack[sp-1] >= (LONG_PTR)stack[sp]); sp--; break;
        case DW_OP_gt:          stack[sp-1] = ((LONG_PTR)stack[sp-1] >  (LONG_PTR)stack[sp]); sp--; break;
        case DW_OP_le:          stack[sp-1] = ((LONG_PTR)stack[sp-1] <= (LONG_PTR)stack[sp]); sp--; break;
        case DW_OP_lt:          stack[sp-1] = ((LONG_PTR)stack[sp-1] <  (LONG_PTR)stack[sp]); sp--; break;
        case DW_OP_eq:          stack[sp-1] = (stack[sp-1] == stack[sp]); sp--; break;
        case DW_OP_ne:          stack[sp-1] = (stack[sp-1] != stack[sp]); sp--; break;
        case DW_OP_skip:        tmp = (short)dwarf_get_u2(&p); p += tmp; break;
        case DW_OP_bra:         tmp = (short)dwarf_get_u2(&p); if (!stack[sp--]) { p += tmp; } break;
        case DW_OP_GNU_encoded_addr: tmp = *p++; stack[++sp] = dwarf_get_ptr( &p, tmp, bases ); break;
        case DW_OP_regx:        stack[++sp] = *(ULONG_PTR *)get_context_reg( context, dwarf_get_uleb128(&p) ); break;
        case DW_OP_bregx:
            reg = dwarf_get_uleb128(&p);
            tmp = dwarf_get_sleb128(&p);
            stack[++sp] = *(ULONG_PTR *)get_context_reg( context, reg ) + tmp;
            break;
        case DW_OP_deref_size:
            switch (*p++)
            {
            case 1: stack[sp] = *(unsigned char *)stack[sp]; break;
            case 2: stack[sp] = *(unsigned short *)stack[sp]; break;
            case 4: stack[sp] = *(unsigned int *)stack[sp]; break;
            case 8: stack[sp] = *(ULONG_PTR *)stack[sp]; break;
            }
            break;
        default:
            FIXME( "unhandled opcode %02x\n", opcode );
        }
    }
    return stack[sp];
}

/* apply the computed frame info to the actual context */
static void apply_frame_state( CONTEXT *context, struct frame_state *state,
                               const struct dwarf_eh_bases *bases )
{
    unsigned int i;
    ULONG_PTR cfa, value;
    CONTEXT new_context = *context;

    switch (state->cfa_rule)
    {
    case RULE_EXPRESSION:
        cfa = *(ULONG_PTR *)eval_expression( (const unsigned char *)state->cfa_offset, context, bases );
        break;
    case RULE_VAL_EXPRESSION:
        cfa = eval_expression( (const unsigned char *)state->cfa_offset, context, bases );
        break;
    default:
        cfa = *(ULONG_PTR *)get_context_reg( context, state->cfa_reg ) + state->cfa_offset;
        break;
    }
    if (!cfa) return;

#ifdef __x86_64__
    new_context.Rsp = cfa;
#elif defined(__aarch64__)
    new_context.Sp = cfa;
#endif

    for (i = 0; i < NB_FRAME_REGS; i++)
    {
        switch (state->rules[i])
        {
        case RULE_UNSET:
        case RULE_UNDEFINED:
        case RULE_SAME:
            break;
        case RULE_CFA_OFFSET:
            set_context_reg( &new_context, i, (char *)cfa + state->regs[i] );
            break;
        case RULE_OTHER_REG:
            set_context_reg( &new_context, i, get_context_reg( context, state->regs[i] ));
            break;
        case RULE_EXPRESSION:
            value = eval_expression( (const unsigned char *)state->regs[i], context, bases );
            set_context_reg( &new_context, i, (void *)value );
            break;
        case RULE_VAL_EXPRESSION:
            value = eval_expression( (const unsigned char *)state->regs[i], context, bases );
            set_context_reg( &new_context, i, &value );
            break;
        }
    }
    *context = new_context;
}

#endif /* NTDLL_DWARF_H_NO_UNWINDER */

#if defined(__i386__)

#define DW_OP_ecx DW_OP_breg1
#define DW_OP_ebx DW_OP_breg3
#define DW_OP_esp DW_OP_breg4
#define DW_OP_ebp DW_OP_breg5

#define DW_REG_eax 0x00
#define DW_REG_ecx 0x01
#define DW_REG_edx 0x02
#define DW_REG_ebx 0x03
#define DW_REG_esp 0x04
#define DW_REG_ebp 0x05
#define DW_REG_esi 0x06
#define DW_REG_edi 0x07
#define DW_REG_eip 0x08

#elif defined(__x86_64__)

#define DW_OP_rcx DW_OP_breg2
#define DW_OP_rbp DW_OP_breg6
#define DW_OP_rsp DW_OP_breg7

#define DW_REG_rbx 0x03
#define DW_REG_rsi 0x04
#define DW_REG_rdi 0x05
#define DW_REG_rbp 0x06
#define DW_REG_rsp 0x07
#define DW_REG_r12 0x0c
#define DW_REG_r13 0x0d
#define DW_REG_r14 0x0e
#define DW_REG_r15 0x0f
#define DW_REG_rip 0x10

#elif defined(__arm__)

#define DW_OP_r0  DW_OP_breg0
#define DW_OP_r1  DW_OP_breg1
#define DW_OP_r2  DW_OP_breg2
#define DW_OP_r3  DW_OP_breg3
#define DW_OP_r4  DW_OP_breg4
#define DW_OP_r5  DW_OP_breg5
#define DW_OP_r6  DW_OP_breg6
#define DW_OP_r7  DW_OP_breg7
#define DW_OP_r8  DW_OP_breg8
#define DW_OP_r9  DW_OP_breg9
#define DW_OP_r10 DW_OP_breg10
#define DW_OP_r11 DW_OP_breg11
#define DW_OP_r12 DW_OP_breg12
#define DW_OP_sp  DW_OP_breg13
#define DW_OP_lr  DW_OP_breg14
#define DW_OP_pc  DW_OP_breg15

#define DW_REG_r0  0
#define DW_REG_r1  1
#define DW_REG_r2  2
#define DW_REG_r3  3
#define DW_REG_r4  4
#define DW_REG_r5  5
#define DW_REG_r6  6
#define DW_REG_r7  7
#define DW_REG_r8  8
#define DW_REG_r9  9
#define DW_REG_r10 10
#define DW_REG_r11 11
#define DW_REG_r12 12
#define DW_REG_sp  13
#define DW_REG_lr  14
#define DW_REG_pc  15

#elif defined(__aarch64__)

#define DW_OP_x19 DW_OP_breg19
#define DW_OP_x20 DW_OP_breg20
#define DW_OP_x21 DW_OP_breg21
#define DW_OP_x22 DW_OP_breg22
#define DW_OP_x23 DW_OP_breg23
#define DW_OP_x24 DW_OP_breg24
#define DW_OP_x25 DW_OP_breg25
#define DW_OP_x26 DW_OP_breg26
#define DW_OP_x27 DW_OP_breg27
#define DW_OP_x28 DW_OP_breg28
#define DW_OP_x29 DW_OP_breg29
#define DW_OP_x30 DW_OP_breg30
#define DW_OP_sp  DW_OP_breg31

#define DW_REG_x19 19
#define DW_REG_x20 20
#define DW_REG_x21 21
#define DW_REG_x22 22
#define DW_REG_x23 23
#define DW_REG_x24 24
#define DW_REG_x25 25
#define DW_REG_x26 26
#define DW_REG_x27 27
#define DW_REG_x28 28
#define DW_REG_x29 29
#define DW_REG_x30 30
#define DW_REG_sp 31
#define DW_REG_v8 72
#define DW_REG_v9 73
#define DW_REG_v10 74
#define DW_REG_v11 75
#define DW_REG_v12 76
#define DW_REG_v13 77
#define DW_REG_v14 78
#define DW_REG_v15 79

#endif /* defined(__aarch64__) */

#define __ASM_CFI_STR(...) #__VA_ARGS__
#define __ASM_CFI_ESC(...) \
    __ASM_CFI(".cfi_escape " __ASM_CFI_STR(__VA_ARGS__) "\n\t")
#define __ASM_CFI_CFA_IS_AT1(base, offset) \
    __ASM_CFI_ESC(DW_CFA_def_cfa_expression, 0x03, DW_OP_ ## base, offset, DW_OP_deref)
#define __ASM_CFI_REG_IS_AT1(reg, base, offset) \
   __ASM_CFI_ESC(DW_CFA_expression, DW_REG_ ## reg, 0x02, DW_OP_ ## base, offset)
#define __ASM_CFI_CFA_IS_AT2(base, lo, hi) \
    __ASM_CFI_ESC(DW_CFA_def_cfa_expression, 0x04, DW_OP_ ## base, lo, hi, DW_OP_deref)
#define __ASM_CFI_REG_IS_AT2(reg, base, lo, hi) \
   __ASM_CFI_ESC(DW_CFA_expression, DW_REG_ ## reg, 0x03, DW_OP_ ## base, lo, hi)

#endif /* __NTDLL_DWARF_H */
