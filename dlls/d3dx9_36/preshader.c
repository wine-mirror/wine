/*
 * Copyright 2016 Paul Gofman
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

#include "config.h"
#include "wine/port.h"

#include "d3dx9_private.h"

#include <float.h>

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

enum pres_ops
{
    PRESHADER_OP_NOP,
    PRESHADER_OP_MOV,
    PRESHADER_OP_ADD,
    PRESHADER_OP_MUL,
    PRESHADER_OP_DOT,
    PRESHADER_OP_NEG,
    PRESHADER_OP_RCP,
    PRESHADER_OP_LT,
    PRESHADER_OP_FRC,
    PRESHADER_OP_MIN,
    PRESHADER_OP_MAX,
    PRESHADER_OP_GE,
    PRESHADER_OP_CMP,
    PRESHADER_OP_SIN,
    PRESHADER_OP_COS,
    PRESHADER_OP_RSQ,
    PRESHADER_OP_EXP,
};

typedef double (*pres_op_func)(double *args, int n);

static double pres_mov(double *args, int n) {return args[0];}
static double pres_add(double *args, int n) {return args[0] + args[1];}
static double pres_mul(double *args, int n) {return args[0] * args[1];}
static double pres_dot(double *args, int n)
{
    int i;
    double sum;

    sum = 0.0;
    for (i = 0; i < n; ++i)
        sum += args[i] * args[i + n];
    return sum;
}
static double pres_neg(double *args, int n) {return -args[0];}
static double pres_rcp(double *args, int n) {return 1.0 / args[0];}
static double pres_lt(double *args, int n)  {return args[0] < args[1] ? 1.0 : 0.0;}
static double pres_ge(double *args, int n)  {return args[0] >= args[1] ? 1.0 : 0.0;}
static double pres_frc(double *args, int n) {return args[0] - floor(args[0]);}
static double pres_min(double *args, int n) {return fmin(args[0], args[1]);}
static double pres_max(double *args, int n) {return fmax(args[0], args[1]);}
static double pres_cmp(double *args, int n) {return args[0] < 0.0 ? args[2] : args[1];}
static double pres_sin(double *args, int n) {return sin(args[0]);}
static double pres_cos(double *args, int n) {return cos(args[0]);}
static double pres_rsq(double *args, int n)
{
    double v;

    v = fabs(args[0]);
    if (v == 0.0)
        return INFINITY;
    else
        return 1.0 / sqrt(v);
}
static double pres_exp(double *args, int n) {return pow(2.0, args[0]);}

#define PRES_OPCODE_MASK 0x7ff00000
#define PRES_OPCODE_SHIFT 20
#define PRES_SCALAR_FLAG 0x80000000
#define PRES_NCOMP_MASK  0x0000ffff

#define FOURCC_PRES 0x53455250
#define FOURCC_CLIT 0x54494c43
#define FOURCC_FXLC 0x434c5846
#define FOURCC_PRSI 0x49535250
#define PRES_SIGN 0x46580000

struct op_info
{
    unsigned int opcode;
    char mnem[8];
    unsigned int input_count;
    BOOL func_all_comps;
    pres_op_func func;
};

static const struct op_info pres_op_info[] =
{
    {0x000, "nop", 0, 0, NULL    }, /* PRESHADER_OP_NOP */
    {0x100, "mov", 1, 0, pres_mov}, /* PRESHADER_OP_MOV */
    {0x204, "add", 2, 0, pres_add}, /* PRESHADER_OP_ADD */
    {0x205, "mul", 2, 0, pres_mul}, /* PRESHADER_OP_MUL */
    {0x500, "dot", 2, 1, pres_dot}, /* PRESHADER_OP_DOT */
    {0x101, "neg", 1, 0, pres_neg}, /* PRESHADER_OP_NEG */
    {0x103, "rcp", 1, 0, pres_rcp}, /* PRESHADER_OP_RCP */
    {0x202, "lt",  2, 0, pres_lt }, /* PRESHADER_OP_LT  */
    {0x104, "frc", 1, 0, pres_frc}, /* PRESHADER_OP_FRC */
    {0x200, "min", 2, 0, pres_min}, /* PRESHADER_OP_MIN */
    {0x201, "max", 2, 0, pres_max}, /* PRESHADER_OP_MAX */
    {0x203, "ge",  2, 0, pres_ge }, /* PRESHADER_OP_GE  */
    {0x300, "cmp", 3, 0, pres_cmp}, /* PRESHADER_OP_CMP */
    {0x108, "sin", 1, 0, pres_sin}, /* PRESHADER_OP_SIN */
    {0x109, "cos", 1, 0, pres_cos}, /* PRESHADER_OP_COS */
    {0x107, "rsq", 1, 0, pres_rsq}, /* PRESHADER_OP_RSQ */
    {0x105, "exp", 1, 0, pres_exp}, /* PRESHADER_OP_EXP */
};

enum pres_value_type
{
    PRES_VT_FLOAT,
    PRES_VT_DOUBLE,
    PRES_VT_INT,
    PRES_VT_BOOL
};

static const struct
{
    unsigned int component_size;
    unsigned int reg_component_count;
    enum pres_value_type type;
}
table_info[] =
{
    {sizeof(double), 1, PRES_VT_DOUBLE}, /* PRES_REGTAB_IMMED */
    {sizeof(float),  4, PRES_VT_FLOAT }, /* PRES_REGTAB_CONST */
    {sizeof(float),  4, PRES_VT_FLOAT }, /* PRES_REGTAB_OCONST */
    {sizeof(BOOL),   1, PRES_VT_BOOL  }, /* PRES_REGTAB_OBCONST */
    {sizeof(int),    4, PRES_VT_INT,  }, /* PRES_REGTAB_OICONST */
    /* TODO: use double precision for 64 bit */
    {sizeof(float),  4, PRES_VT_FLOAT }  /* PRES_REGTAB_TEMP */
};

static const char *table_symbol[] =
{
    "imm", "c", "oc", "ob", "oi", "r", "(null)",
};

static const enum pres_reg_tables pres_regset2table[] =
{
    PRES_REGTAB_OBCONST,  /* D3DXRS_BOOL */
    PRES_REGTAB_OICONST,  /* D3DXRS_INT4 */
    PRES_REGTAB_CONST,    /* D3DXRS_FLOAT4 */
    PRES_REGTAB_COUNT,     /* D3DXRS_SAMPLER */
};

static const enum pres_reg_tables shad_regset2table[] =
{
    PRES_REGTAB_OBCONST,  /* D3DXRS_BOOL */
    PRES_REGTAB_OICONST,  /* D3DXRS_INT4 */
    PRES_REGTAB_OCONST,   /* D3DXRS_FLOAT4 */
    PRES_REGTAB_COUNT,     /* D3DXRS_SAMPLER */
};

struct d3dx_pres_operand
{
    enum pres_reg_tables table;
    /* offset is component index, not register index, e. g.
       offset for component c3.y is 13 (3 * 4 + 1) */
    unsigned int offset;
};

#define MAX_INPUTS_COUNT 3

struct d3dx_pres_ins
{
    enum pres_ops op;
    /* first input argument is scalar,
       scalar component is propagated */
    BOOL scalar_op;
    unsigned int component_count;
    struct d3dx_pres_operand inputs[MAX_INPUTS_COUNT];
    struct d3dx_pres_operand output;
};

static unsigned int get_reg_offset(unsigned int table, unsigned int offset)
{
    return offset / table_info[table].reg_component_count;
}

#define PRES_BITMASK_BLOCK_SIZE (sizeof(unsigned int) * 8)

static HRESULT regstore_alloc_table(struct d3dx_regstore *rs, unsigned int table)
{
    unsigned int size;

    size = rs->table_sizes[table] * table_info[table].reg_component_count * table_info[table].component_size;
    if (size)
    {
        rs->tables[table] = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
        rs->table_value_set[table] = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                sizeof(*rs->table_value_set[table]) *
                ((rs->table_sizes[table] + PRES_BITMASK_BLOCK_SIZE - 1) / PRES_BITMASK_BLOCK_SIZE));
        if (!rs->tables[table] || !rs->table_value_set[table])
            return E_OUTOFMEMORY;
    }
    return D3D_OK;
}

static void regstore_free_tables(struct d3dx_regstore *rs)
{
    unsigned int i;

    for (i = 0; i < PRES_REGTAB_COUNT; ++i)
    {
        HeapFree(GetProcessHeap(), 0, rs->tables[i]);
        HeapFree(GetProcessHeap(), 0, rs->table_value_set[i]);
    }
}

static void regstore_set_values(struct d3dx_regstore *rs, unsigned int table, void *data,
        unsigned int start_offset, unsigned int count)
{
    unsigned int block_idx, start, end, start_block, end_block;

    if (!count)
        return;

    memcpy((BYTE *)rs->tables[table] + start_offset * table_info[table].component_size,
            data, count * table_info[table].component_size);

    start = get_reg_offset(table, start_offset);
    start_block = start / PRES_BITMASK_BLOCK_SIZE;
    start -= start_block * PRES_BITMASK_BLOCK_SIZE;
    end = get_reg_offset(table, start_offset + count - 1);
    end_block = end / PRES_BITMASK_BLOCK_SIZE;
    end = (end_block + 1) * PRES_BITMASK_BLOCK_SIZE - 1 - end;

    if (start_block == end_block)
    {
        rs->table_value_set[table][start_block] |= (~0u << start) & (~0u >> end);
    }
    else
    {
        rs->table_value_set[table][start_block] |= ~0u << start;

        for (block_idx = start_block + 1; block_idx < end_block; ++block_idx)
            rs->table_value_set[table][block_idx] = ~0u;

        rs->table_value_set[table][end_block] |= ~0u >> end;
    }
}

static unsigned int regstore_is_val_set_reg(struct d3dx_regstore *rs, unsigned int table, unsigned int reg_idx)
{
    return rs->table_value_set[table][reg_idx / PRES_BITMASK_BLOCK_SIZE] &
            (1u << (reg_idx % PRES_BITMASK_BLOCK_SIZE));
}

static double regstore_get_double(struct d3dx_regstore *rs, unsigned int table, unsigned int offset)
{
    BYTE *p;

    p = (BYTE *)rs->tables[table] + table_info[table].component_size * offset;
    switch (table_info[table].type)
    {
        case PRES_VT_FLOAT:
            return *(float *)p;
        case PRES_VT_DOUBLE:
            return *(double *)p;
        default:
            FIXME("Unexpected preshader input from table %u.\n", table);
            return NAN;
    }
}

static void regstore_set_double(struct d3dx_regstore *rs, unsigned int table, unsigned int offset, double v)
{
    BYTE *p;
    unsigned int reg_idx;

    p = (BYTE *)rs->tables[table] + table_info[table].component_size * offset;
    switch (table_info[table].type)
    {
        case PRES_VT_FLOAT : *(float *)p = v; break;
        case PRES_VT_DOUBLE: *(double *)p = v; break;
        case PRES_VT_INT   : *(int *)p = lrint(v); break;
        case PRES_VT_BOOL  : *(BOOL *)p = !!v; break;
    }
    reg_idx = get_reg_offset(table, offset);
    rs->table_value_set[table][reg_idx / PRES_BITMASK_BLOCK_SIZE] |=
            1u << (reg_idx % PRES_BITMASK_BLOCK_SIZE);
}

static void regstore_reset_table(struct d3dx_regstore *rs, unsigned int table)
{
    unsigned int size;

    size = rs->table_sizes[table] * table_info[table].reg_component_count * table_info[table].component_size;

    memset(rs->tables[table], 0, size);
    memset(rs->table_value_set[table], 0,
            sizeof(*rs->table_value_set[table]) *
            ((rs->table_sizes[table] + PRES_BITMASK_BLOCK_SIZE - 1) / PRES_BITMASK_BLOCK_SIZE));
}

static void dump_bytecode(void *data, unsigned int size)
{
    unsigned int *bytecode = (unsigned int *)data;
    unsigned int i, j, n;

    size /= sizeof(*bytecode);
    i = 0;
    while (i < size)
    {
        n = min(size - i, 8);
        for (j = 0; j < n; ++j)
            TRACE("0x%08x,", bytecode[i + j]);
        i += n;
        TRACE("\n");
    }
}

static unsigned int *find_bytecode_comment(unsigned int *ptr, unsigned int count,
        unsigned int fourcc, unsigned int *size)
{
    /* Provide at least one value in comment section on non-NULL return. */
    while (count > 2 && (*ptr & 0xffff) == 0xfffe)
    {
        unsigned int section_size;

        section_size = (*ptr >> 16);
        if (!section_size || section_size + 1 > count)
            break;
        if (*(ptr + 1) == fourcc)
        {
            *size = section_size;
            return ptr + 2;
        }
        count -= section_size + 1;
        ptr += section_size + 1;
    }
    return NULL;
}

static unsigned int *parse_pres_arg(unsigned int *ptr, unsigned int count, struct d3dx_pres_operand *opr)
{
    static const enum pres_reg_tables reg_table[8] =
    {
        PRES_REGTAB_COUNT, PRES_REGTAB_IMMED, PRES_REGTAB_CONST, PRES_REGTAB_COUNT,
        PRES_REGTAB_OCONST, PRES_REGTAB_OBCONST, PRES_REGTAB_OICONST, PRES_REGTAB_TEMP
    };

    if (count < 3)
    {
        WARN("Byte code buffer ends unexpectedly.\n");
        return NULL;
    }

    if (*ptr)
    {
        FIXME("Relative addressing not supported yet, word %#x.\n", *ptr);
        return NULL;
    }
    ++ptr;

    if (*ptr >= ARRAY_SIZE(reg_table) || reg_table[*ptr] == PRES_REGTAB_COUNT)
    {
        FIXME("Unsupported register table %#x.\n", *ptr);
        return NULL;
    }
    opr->table = reg_table[*ptr++];
    opr->offset = *ptr++;

    if (opr->table == PRES_REGTAB_OBCONST)
        opr->offset /= 4;
    return ptr;
}

static unsigned int *parse_pres_ins(unsigned int *ptr, unsigned int count, struct d3dx_pres_ins *ins)
{
    unsigned int ins_code, ins_raw;
    unsigned int input_count;
    unsigned int i;

    if (count < 2)
    {
        WARN("Byte code buffer ends unexpectedly.\n");
        return NULL;
    }

    ins_raw = *ptr++;
    ins_code = (ins_raw & PRES_OPCODE_MASK) >> PRES_OPCODE_SHIFT;
    ins->component_count = ins_raw & PRES_NCOMP_MASK;
    ins->scalar_op = !!(ins_raw & PRES_SCALAR_FLAG);

    if (ins->component_count < 1 || ins->component_count > 4)
    {
        FIXME("Unsupported number of components %u.\n", ins->component_count);
        return NULL;
    }
    input_count = *ptr++;
    count -= 2;
    for (i = 0; i < ARRAY_SIZE(pres_op_info); ++i)
        if (ins_code == pres_op_info[i].opcode)
            break;
    if (i == ARRAY_SIZE(pres_op_info))
    {
        FIXME("Unknown opcode %#x, raw %#x.\n", ins_code, ins_raw);
        return NULL;
    }
    ins->op = i;
    if (input_count > ARRAY_SIZE(ins->inputs) || input_count != pres_op_info[i].input_count)
    {
        FIXME("Actual input args %u, expected %u, instruction %s.\n", input_count,
                pres_op_info[i].input_count, pres_op_info[i].mnem);
        return NULL;
    }
    for (i = 0; i < input_count; ++i)
    {
        unsigned int *p;

        p = parse_pres_arg(ptr, count, &ins->inputs[i]);
        if (!p)
            return NULL;
        count -= p - ptr;
        ptr = p;
    }
    ptr = parse_pres_arg(ptr, count, &ins->output);
    return ptr;
}

static HRESULT get_ctab_constant_desc(ID3DXConstantTable *ctab, D3DXHANDLE hc, D3DXCONSTANT_DESC *desc)
{
    D3DXCONSTANT_DESC buffer[2];
    HRESULT hr;
    unsigned int count;

    count = ARRAY_SIZE(buffer);
    if (FAILED(hr = ID3DXConstantTable_GetConstantDesc(ctab, hc, buffer, &count)))
    {
        FIXME("Could not get constant desc, hr %#x.\n", hr);
        return hr;
    }
    else if (count != 1)
    {
        FIXME("Unexpected constant descriptors count %u.\n", count);
        return D3DERR_INVALIDCALL;
    }
    *desc = buffer[0];
    return D3D_OK;
}

static HRESULT get_constants_desc(unsigned int *byte_code, struct d3dx_const_tab *out, struct d3dx9_base_effect *base)
{
    ID3DXConstantTable *ctab;
    D3DXCONSTANT_DESC *cdesc;
    struct d3dx_parameter **inputs_param;
    D3DXCONSTANTTABLE_DESC desc;
    HRESULT hr;
    D3DXHANDLE hc;
    unsigned int i;

    out->inputs = cdesc = NULL;
    out->ctab = NULL;
    out->inputs_param = NULL;
    out->input_count = 0;
    inputs_param = NULL;
    hr = D3DXGetShaderConstantTable(byte_code, &ctab);
    if (FAILED(hr) || !ctab)
    {
        TRACE("Could not get CTAB data, hr %#x.\n", hr);
        /* returning OK, shaders and preshaders without CTAB are valid */
        return D3D_OK;
    }
    if (FAILED(hr = ID3DXConstantTable_GetDesc(ctab, &desc)))
    {
        FIXME("Could not get CTAB desc, hr %#x.\n", hr);
        goto err_out;
    }

    cdesc = HeapAlloc(GetProcessHeap(), 0, sizeof(*cdesc) * desc.Constants);
    inputs_param = HeapAlloc(GetProcessHeap(), 0, sizeof(*inputs_param) * desc.Constants);
    if (!cdesc || !inputs_param)
    {
        hr = E_OUTOFMEMORY;
        goto err_out;
    }

    for (i = 0; i < desc.Constants; ++i)
    {
        hc = ID3DXConstantTable_GetConstant(ctab, NULL, i);
        if (!hc)
        {
            FIXME("Null constant handle.\n");
            goto err_out;
        }
        if (FAILED(hr = get_ctab_constant_desc(ctab, hc, &cdesc[i])))
            goto err_out;
        inputs_param[i] = get_parameter_by_name(base, NULL, cdesc[i].Name);
        if (cdesc[i].Class == D3DXPC_OBJECT)
            TRACE("Object %s, parameter %p.\n", cdesc[i].Name, inputs_param[i]);
        else if (!inputs_param[i])
            ERR("Could not find parameter %s in effect.\n", cdesc[i].Name);
    }
    out->input_count = desc.Constants;
    out->inputs = cdesc;
    out->inputs_param = inputs_param;
    out->ctab = ctab;
    return D3D_OK;

err_out:
    HeapFree(GetProcessHeap(), 0, cdesc);
    HeapFree(GetProcessHeap(), 0, inputs_param);
    if (ctab)
        ID3DXConstantTable_Release(ctab);
    return hr;
}

static void update_table_size(unsigned int *table_sizes, unsigned int table, unsigned int max_register)
{
    if (table < PRES_REGTAB_COUNT)
        table_sizes[table] = max(table_sizes[table], max_register + 1);
}

static void update_table_sizes_consts(unsigned int *table_sizes, struct d3dx_const_tab *ctab)
{
    unsigned int i, table, max_register;

    for (i = 0; i < ctab->input_count; ++i)
    {
        if (!ctab->inputs[i].RegisterCount)
            continue;
        max_register = ctab->inputs[i].RegisterIndex + ctab->inputs[i].RegisterCount - 1;
        table = ctab->regset2table[ctab->inputs[i].RegisterSet];
        update_table_size(table_sizes, table, max_register);
    }
}

static void dump_arg(struct d3dx_regstore *rs, const struct d3dx_pres_operand *arg, int component_count)
{
    static const char *xyzw_str = "xyzw";
    unsigned int i, table;

    table = arg->table;
    if (table == PRES_REGTAB_IMMED)
    {
        TRACE("(");
        for (i = 0; i < component_count; ++i)
            TRACE(i < component_count - 1 ? "%.16e, " : "%.16e",
                    ((double *)rs->tables[PRES_REGTAB_IMMED])[arg->offset + i]);
        TRACE(")");
    }
    else
    {
        TRACE("%s%u.", table_symbol[table], get_reg_offset(table, arg->offset));
        for (i = 0; i < component_count; ++i)
            TRACE("%c", xyzw_str[(arg->offset + i) % 4]);
    }
}

static void dump_registers(struct d3dx_const_tab *ctab)
{
    unsigned int table, i;

    for (i = 0; i < ctab->input_count; ++i)
    {
        table = ctab->regset2table[ctab->inputs[i].RegisterSet];
        TRACE("//   %-12s %s%-4u %u\n", ctab->inputs_param[i] ? ctab->inputs_param[i]->name : "(nil)",
                table_symbol[table], ctab->inputs[i].RegisterIndex, ctab->inputs[i].RegisterCount);
    }
}

static void dump_ins(struct d3dx_regstore *rs, const struct d3dx_pres_ins *ins)
{
    unsigned int i;

    TRACE("    %s ", pres_op_info[ins->op].mnem);
    dump_arg(rs, &ins->output, pres_op_info[ins->op].func_all_comps ? 1 : ins->component_count);
    for (i = 0; i < pres_op_info[ins->op].input_count; ++i)
    {
        TRACE(", ");
        dump_arg(rs, &ins->inputs[i], ins->scalar_op && !i ? 1 : ins->component_count);
    }
    TRACE("\n");
}

static void dump_preshader(struct d3dx_preshader *pres)
{
    unsigned int i;

    TRACE("// Preshader registers:\n");
    dump_registers(&pres->inputs);
    TRACE("    preshader\n");
    for (i = 0; i < pres->ins_count; ++i)
        dump_ins(&pres->regs, &pres->ins[i]);
}

static HRESULT parse_preshader(struct d3dx_preshader *pres, unsigned int *ptr, unsigned int count, struct d3dx9_base_effect *base)
{
    unsigned int *p;
    unsigned int i, j, const_count;
    double *dconst;
    HRESULT hr;
    unsigned int saved_word;
    unsigned int section_size;

    TRACE("Preshader version %#x.\n", *ptr & 0xffff);

    if (!count)
    {
        WARN("Unexpected end of byte code buffer.\n");
        return D3DXERR_INVALIDDATA;
    }

    p = find_bytecode_comment(ptr + 1, count - 1, FOURCC_CLIT, &section_size);
    if (p)
    {
        const_count = *p++;
        if (const_count > (section_size - 1) / (sizeof(double) / sizeof(unsigned int)))
        {
            WARN("Byte code buffer ends unexpectedly.\n");
            return D3DXERR_INVALIDDATA;
        }
        dconst = (double *)p;
    }
    else
    {
        const_count = 0;
        dconst = NULL;
    }
    TRACE("%u double constants.\n", const_count);

    p = find_bytecode_comment(ptr + 1, count - 1, FOURCC_FXLC, &section_size);
    if (!p)
    {
        WARN("Could not find preshader code.\n");
        return D3D_OK;
    }
    pres->ins_count = *p++;
    --section_size;
    if (pres->ins_count > UINT_MAX / sizeof(*pres->ins))
    {
        WARN("Invalid instruction count %u.\n", pres->ins_count);
        return D3DXERR_INVALIDDATA;
    }
    TRACE("%u instructions.\n", pres->ins_count);
    pres->ins = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pres->ins) * pres->ins_count);
    if (!pres->ins)
        return E_OUTOFMEMORY;
    for (i = 0; i < pres->ins_count; ++i)
    {
        unsigned int *ptr_next;

        ptr_next = parse_pres_ins(p, section_size, &pres->ins[i]);
        if (!ptr_next)
            return D3DXERR_INVALIDDATA;
        section_size -= ptr_next - p;
        p = ptr_next;
    }

    saved_word = *ptr;
    *ptr = 0xfffe0000;
    hr = get_constants_desc(ptr, &pres->inputs, base);
    *ptr = saved_word;
    if (FAILED(hr))
        return hr;

    pres->inputs.regset2table = pres_regset2table;

    pres->regs.table_sizes[PRES_REGTAB_IMMED] = const_count;

    for (i = 0; i < pres->ins_count; ++i)
    {
        for (j = 0; j < pres_op_info[pres->ins[i].op].input_count; ++j)
            update_table_size(pres->regs.table_sizes, pres->ins[i].inputs[j].table,
                    get_reg_offset(pres->ins[i].inputs[j].table,
                    pres->ins[i].inputs[j].offset + pres->ins[i].component_count - 1));

        update_table_size(pres->regs.table_sizes, pres->ins[i].output.table,
                get_reg_offset(pres->ins[i].output.table,
                pres->ins[i].output.offset + pres->ins[i].component_count - 1));
    }
    update_table_sizes_consts(pres->regs.table_sizes, &pres->inputs);
    if (FAILED(regstore_alloc_table(&pres->regs, PRES_REGTAB_IMMED)))
        return E_OUTOFMEMORY;
    regstore_set_values(&pres->regs, PRES_REGTAB_IMMED, dconst, 0, const_count);

    return D3D_OK;
}

void d3dx_create_param_eval(struct d3dx9_base_effect *base_effect, void *byte_code, unsigned int byte_code_size,
        D3DXPARAMETER_TYPE type, struct d3dx_param_eval **peval_out)
{
    struct d3dx_param_eval *peval;
    unsigned int *ptr;
    HRESULT hr;
    unsigned int i;
    BOOL shader;
    unsigned int count, pres_size;

    TRACE("base_effect %p, byte_code %p, byte_code_size %u, type %u, peval_out %p.\n",
            base_effect, byte_code, byte_code_size, type, peval_out);

    count = byte_code_size / sizeof(unsigned int);
    if (!byte_code || !count)
    {
        *peval_out = NULL;
        return;
    }

    peval = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*peval));
    if (!peval)
        goto err_out;

    peval->param_type = type;
    switch (type)
    {
        case D3DXPT_VERTEXSHADER:
        case D3DXPT_PIXELSHADER:
            shader = TRUE;
            break;
        default:
            shader = FALSE;
            break;
    }
    peval->shader_inputs.regset2table = shad_regset2table;

    ptr = (unsigned int *)byte_code;
    if (shader)
    {
        if ((*ptr & 0xfffe0000) != 0xfffe0000)
        {
            FIXME("Invalid shader signature %#x.\n", *ptr);
            goto err_out;
        }
        TRACE("Shader version %#x.\n", *ptr & 0xffff);

        if (FAILED(hr = get_constants_desc(ptr, &peval->shader_inputs, base_effect)))
        {
            FIXME("Could not get shader constant table, hr %#x.\n", hr);
            goto err_out;
        }
        update_table_sizes_consts(peval->pres.regs.table_sizes, &peval->shader_inputs);
        ptr = find_bytecode_comment(ptr + 1, count - 1, FOURCC_PRES, &pres_size);
        if (!ptr)
            TRACE("No preshader found.\n");
    }
    else
    {
        pres_size = count;
    }

    if (ptr && FAILED(parse_preshader(&peval->pres, ptr, pres_size, base_effect)))
    {
        FIXME("Failed parsing preshader, byte code for analysis follows.\n");
        dump_bytecode(byte_code, byte_code_size);
        goto err_out;
    }

    for (i = PRES_REGTAB_FIRST_SHADER; i < PRES_REGTAB_COUNT; ++i)
    {
        if (FAILED(regstore_alloc_table(&peval->pres.regs, i)))
            goto err_out;
    }

    if (TRACE_ON(d3dx))
    {
        dump_bytecode(byte_code, byte_code_size);
        dump_preshader(&peval->pres);
        if (shader)
        {
            TRACE("// Shader registers:\n");
            dump_registers(&peval->shader_inputs);
        }
    }
    *peval_out = peval;
    TRACE("Created parameter evaluator %p.\n", *peval_out);
    return;

err_out:
    FIXME("Error creating parameter evaluator.\n");
    d3dx_free_param_eval(peval);
    *peval_out = NULL;
}

static void d3dx_free_const_tab(struct d3dx_const_tab *ctab)
{
    HeapFree(GetProcessHeap(), 0, ctab->inputs);
    HeapFree(GetProcessHeap(), 0, ctab->inputs_param);
    if (ctab->ctab)
        ID3DXConstantTable_Release(ctab->ctab);
}

static void d3dx_free_preshader(struct d3dx_preshader *pres)
{
    HeapFree(GetProcessHeap(), 0, pres->ins);

    regstore_free_tables(&pres->regs);
    d3dx_free_const_tab(&pres->inputs);
}

void d3dx_free_param_eval(struct d3dx_param_eval *peval)
{
    TRACE("peval %p.\n", peval);

    if (!peval)
        return;

    d3dx_free_preshader(&peval->pres);
    d3dx_free_const_tab(&peval->shader_inputs);
    HeapFree(GetProcessHeap(), 0, peval);
}

static HRESULT set_constants_param(struct d3dx_regstore *rs, struct d3dx_const_tab *const_tab,
        D3DXHANDLE hc, struct d3dx_parameter *param)
{
    ID3DXConstantTable *ctab = const_tab->ctab;
    D3DXCONSTANT_DESC desc;
    unsigned int const_count, param_count, i, j, n, table, start_offset;
    unsigned int minor, major, major_stride, param_offset;
    BOOL transpose, get_element;

    if (FAILED(get_ctab_constant_desc(ctab, hc, &desc)))
        return D3DERR_INVALIDCALL;

    if (param->element_count)
    {
        param_count = param->element_count;
        const_count = desc.Elements;
        get_element = TRUE;
    }
    else
    {
        if (desc.Elements > 1)
        {
            FIXME("Unexpected number of constant elements %u.\n", desc.Elements);
            return D3DERR_INVALIDCALL;
        }
        param_count = param->member_count;
        const_count = desc.StructMembers;
        get_element = FALSE;
    }
    if (const_count != param_count)
    {
        FIXME("Number of elements or struct members differs between parameter (%u) and constant (%u).\n",
                param_count, const_count);
        return D3DERR_INVALIDCALL;
    }
    if (const_count)
    {
        HRESULT hr, ret;
        D3DXHANDLE hc_element;

        ret = D3D_OK;
        for (i = 0; i < const_count; ++i)
        {
            if (get_element)
                hc_element = ID3DXConstantTable_GetConstantElement(ctab, hc, i);
            else
                hc_element = ID3DXConstantTable_GetConstant(ctab, hc, i);
            if (!hc_element)
            {
                FIXME("Could not get constant.\n");
                hr = D3DERR_INVALIDCALL;
            }
            else
            {
                hr = set_constants_param(rs, const_tab, hc_element, &param->members[i]);
            }
            if (FAILED(hr))
                ret = hr;
        }
        return ret;
    }

    transpose = (desc.Class == D3DXPC_MATRIX_COLUMNS && param->class == D3DXPC_MATRIX_ROWS)
            || (param->class == D3DXPC_MATRIX_COLUMNS && desc.Class == D3DXPC_MATRIX_ROWS);
    if (desc.Class == D3DXPC_MATRIX_COLUMNS)
    {
        major = param->columns;
        minor = param->rows;
    }
    else
    {
        major = param->rows;
        minor = param->columns;
    }

    TRACE("Constant %s, rows %u, columns %u, class %u, bytes %u.\n",
            debugstr_a(desc.Name), desc.Rows, desc.Columns, desc.Class, desc.Bytes);
    TRACE("Parameter %s, rows %u, columns %u, class %u, flags %#x, bytes %u, transpose %#x.\n",
            debugstr_a(param->name), param->rows, param->columns, param->class,
            param->flags, param->bytes, transpose);

    if (desc.RegisterSet >= ARRAY_SIZE(shad_regset2table))
    {
        FIXME("Unknown register set %u.\n", desc.RegisterSet);
        return D3DERR_INVALIDCALL;
    }
    table = const_tab->regset2table[desc.RegisterSet];
    if (table >= PRES_REGTAB_COUNT)
    {
        ERR("Unexpected register set %u.\n", desc.RegisterSet);
        return D3DERR_INVALIDCALL;
    }
    start_offset = desc.RegisterIndex * table_info[table].reg_component_count;
    major_stride = max(minor, table_info[table].reg_component_count);
    n = min(major * major_stride,
            desc.RegisterCount * table_info[table].reg_component_count + major_stride - 1) / major_stride;
    for (i = 0; i < n; ++i)
    {
        for (j = 0; j < minor; ++j)
        {
            unsigned int out;
            unsigned int *in;
            unsigned int offset;

            offset = start_offset + i * major_stride + j;
            if (offset / table_info[table].reg_component_count >= rs->table_sizes[table])
            {
                if (table_info[table].reg_component_count != 1)
                    FIXME("Output offset exceeds table size, name %s, component %u.\n", desc.Name, i);
                break;
            }
            if (transpose)
                param_offset = i + j * major;
            else
                param_offset = i * minor + j;
            if (param_offset * sizeof(unsigned int) >= param->bytes)
            {
                WARN("Parameter data is too short, name %s, component %u.\n", desc.Name, i);
                break;
            }

            in = (unsigned int *)param->data + param_offset;
            /* TODO: store data transfer / convert operation instead of performing an operation
                from here, to move this to parsing stage */
            switch (table_info[table].type)
            {
                case PRES_VT_FLOAT: set_number(&out, D3DXPT_FLOAT, in, param->type); break;
                case PRES_VT_INT: set_number(&out, D3DXPT_INT, in, param->type); break;
                case PRES_VT_BOOL: set_number(&out, D3DXPT_BOOL, in, param->type); break;
                default:
                    FIXME("Unexpected type %#x.\n", table_info[table].type);
                    break;
            }
            regstore_set_values(rs, table, &out, offset, 1);
        }
    }

    return D3D_OK;
}

static HRESULT set_constants(struct d3dx_regstore *rs, struct d3dx_const_tab *const_tab)
{
    unsigned int i;
    HRESULT hr, ret;
    D3DXHANDLE hc;

    ret = D3D_OK;
    for (i = 0; i < const_tab->input_count; ++i)
    {
        if (!const_tab->inputs_param[i] || const_tab->inputs_param[i]->class == D3DXPC_OBJECT)
            continue;
        hc = ID3DXConstantTable_GetConstant(const_tab->ctab, NULL, i);
        if (hc)
        {
            hr = set_constants_param(rs, const_tab, hc, const_tab->inputs_param[i]);
        }
        else
        {
            FIXME("Could not get constant, index %u.\n", i);
            hr = D3DERR_INVALIDCALL;
        }
        if (FAILED(hr))
            ret = hr;
    }
    return ret;
}

static double exec_get_arg(struct d3dx_regstore *rs, const struct d3dx_pres_ins *ins,
        const struct d3dx_pres_operand *opr, unsigned int comp)
{
    if (!regstore_is_val_set_reg(rs, opr->table, (opr->offset + comp) / table_info[opr->table].reg_component_count))
    {
        WARN("Using uninitialized input ");
        dump_arg(rs, opr, comp);
        TRACE(".\n");
        dump_ins(rs, ins);
    }
    return regstore_get_double(rs, opr->table, opr->offset + comp);
}

static void exec_set_arg(struct d3dx_regstore *rs, const struct d3dx_pres_operand *opr,
        unsigned int comp, double res)
{
    regstore_set_double(rs, opr->table, opr->offset + comp, res);
}

#define ARGS_ARRAY_SIZE 8
static HRESULT execute_preshader(struct d3dx_preshader *pres)
{
    unsigned int i, j, k;
    double args[ARGS_ARRAY_SIZE];
    double res;

    for (i = 0; i < pres->ins_count; ++i)
    {
        const struct d3dx_pres_ins *ins;
        const struct op_info *oi;

        ins = &pres->ins[i];
        oi = &pres_op_info[ins->op];
        if (oi->func_all_comps)
        {
            if (oi->input_count * ins->component_count > ARGS_ARRAY_SIZE)
            {
                FIXME("Too many arguments (%u) for one instruction.\n", oi->input_count * ins->component_count);
                return E_FAIL;
            }
            for (k = 0; k < oi->input_count; ++k)
                for (j = 0; j < ins->component_count; ++j)
                    args[k * ins->component_count + j] = exec_get_arg(&pres->regs, ins, &ins->inputs[k],
                            ins->scalar_op && !k ? 0 : j);
            res = oi->func(args, ins->component_count);

            /* only 'dot' instruction currently falls here */
            exec_set_arg(&pres->regs, &ins->output, 0, res);
        }
        else
        {
            for (j = 0; j < ins->component_count; ++j)
            {
                for (k = 0; k < oi->input_count; ++k)
                    args[k] = exec_get_arg(&pres->regs, ins, &ins->inputs[k], ins->scalar_op && !k ? 0 : j);
                res = oi->func(args, ins->component_count);
                exec_set_arg(&pres->regs, &ins->output, j, res);
            }
        }
    }
    return D3D_OK;
}

HRESULT d3dx_evaluate_parameter(struct d3dx_param_eval *peval, const struct d3dx_parameter *param, void *param_value)
{
    HRESULT hr;
    unsigned int i;
    unsigned int elements, elements_param, elements_table;
    float *oc;

    TRACE("peval %p, param %p, param_value %p.\n", peval, param, param_value);

    if (FAILED(hr = set_constants(&peval->pres.regs, &peval->pres.inputs)))
        return hr;

    if (FAILED(hr = execute_preshader(&peval->pres)))
        return hr;

    elements_table = table_info[PRES_REGTAB_OCONST].reg_component_count
            * peval->pres.regs.table_sizes[PRES_REGTAB_OCONST];
    elements_param = param->bytes / sizeof(unsigned int);
    elements = min(elements_table, elements_param);
    oc = (float *)peval->pres.regs.tables[PRES_REGTAB_OCONST];
    for (i = 0; i < elements; ++i)
        set_number((unsigned int *)param_value + i, param->type, oc + i, D3DXPT_FLOAT);
    return D3D_OK;
}

static HRESULT set_shader_constants_device(struct IDirect3DDevice9 *device, struct d3dx_regstore *rs,
        D3DXPARAMETER_TYPE type, enum pres_reg_tables table)
{
    unsigned int start, count;
    void *ptr;
    HRESULT hr, result;

    result = D3D_OK;
    start = 0;
    while (start < rs->table_sizes[table])
    {
        count = 0;
        while (start < rs->table_sizes[table] && !regstore_is_val_set_reg(rs, table, start))
            ++start;
        while (start + count < rs->table_sizes[table] && regstore_is_val_set_reg(rs, table, start + count))
            ++count;
        if (!count)
            break;
        TRACE("Setting %u constants at %u.\n", count, start);
        ptr = (BYTE *)rs->tables[table] + start * table_info[table].reg_component_count
                * table_info[table].component_size;
        if (type == D3DXPT_VERTEXSHADER)
        {
            switch(table)
            {
                case PRES_REGTAB_OCONST:
                    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, start, (const float *)ptr, count);
                    break;
                case PRES_REGTAB_OICONST:
                    hr = IDirect3DDevice9_SetVertexShaderConstantI(device, start, (const int *)ptr, count);
                    break;
                case PRES_REGTAB_OBCONST:
                    hr = IDirect3DDevice9_SetVertexShaderConstantB(device, start, (const BOOL *)ptr, count);
                    break;
                default:
                    FIXME("Unexpected register table %u.\n", table);
                    return D3DERR_INVALIDCALL;
            }
        }
        else if (type == D3DXPT_PIXELSHADER)
        {
            switch(table)
            {
                case PRES_REGTAB_OCONST:
                    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, start, (const float *)ptr, count);
                    break;
                case PRES_REGTAB_OICONST:
                    hr = IDirect3DDevice9_SetPixelShaderConstantI(device, start, (const int *)ptr, count);
                    break;
                case PRES_REGTAB_OBCONST:
                    hr = IDirect3DDevice9_SetPixelShaderConstantB(device, start, (const BOOL *)ptr, count);
                    break;
                default:
                    FIXME("Unexpected register table %u.\n", table);
                    return D3DERR_INVALIDCALL;
            }
        }
        else
        {
            FIXME("Unexpected parameter type %u.\n", type);
            return D3DERR_INVALIDCALL;
        }

        if (FAILED(hr))
        {
            ERR("Setting constants failed, type %u, table %u, hr %#x.\n", type, table, hr);
            result = hr;
        }
        start += count;
    }
    regstore_reset_table(rs, table);
    return result;
}

HRESULT d3dx_param_eval_set_shader_constants(struct IDirect3DDevice9 *device, struct d3dx_param_eval *peval)
{
    static const enum pres_reg_tables set_tables[] =
            {PRES_REGTAB_OCONST, PRES_REGTAB_OICONST, PRES_REGTAB_OBCONST};
    HRESULT hr, result;
    struct d3dx_preshader *pres = &peval->pres;
    struct d3dx_regstore *rs = &pres->regs;
    unsigned int i;

    TRACE("device %p, peval %p, param_type %u.\n", device, peval, peval->param_type);

    if (FAILED(hr = set_constants(rs, &pres->inputs)))
        return hr;
    if (FAILED(hr = execute_preshader(pres)))
        return hr;
    if (FAILED(hr = set_constants(rs, &peval->shader_inputs)))
        return hr;

    result = D3D_OK;
    for (i = 0; i < ARRAY_SIZE(set_tables); ++i)
    {
        if (FAILED(hr = set_shader_constants_device(device, rs, peval->param_type, set_tables[i])))
            result = hr;
    }
    return result;
}
